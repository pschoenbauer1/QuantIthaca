"""Write pandas DataFrames to PostgreSQL with strict validation."""

from __future__ import annotations

import math
from datetime import date, datetime, time, timezone
from typing import Any, Literal

import numpy as np
import pandas as pd
from sqlalchemy import Date, DateTime, inspect, insert
from sqlalchemy.engine import Connection, Engine
from sqlalchemy.schema import MetaData, Table
from sqlalchemy.types import TIMESTAMP

from quantithaca.utils.dateutils import to_date, to_datetime

try:
    from sqlalchemy.dialects.postgresql import insert as pg_insert
except ImportError:  # pragma: no cover
    pg_insert = None

__all__ = ["sql_write"]


def _assert_no_missing_or_bad_values(df: pd.DataFrame) -> None:
    if df.empty:
        raise ValueError("DataFrame is empty.")

    if df.isna().any().any():
        bad = df.columns[df.isna().any()].tolist()
        raise ValueError(f"Null, NaT, or NA values are not allowed in columns: {bad}")

    for col in df.columns:
        s = df[col]
        if pd.api.types.is_float_dtype(s):
            arr = s.to_numpy(copy=False, dtype=np.float64)
            if not np.isfinite(arr).all():
                raise ValueError(
                    f"Non-finite values (NaN or inf) are not allowed in column {col!r}."
                )
        elif pd.api.types.is_complex_dtype(s):
            arr = s.to_numpy(copy=False)
            if not (np.isfinite(arr.real).all() and np.isfinite(arr.imag).all()):
                raise ValueError(f"Non-finite complex values are not allowed in column {col!r}.")
        elif s.dtype == object:
            for idx, v in s.items():
                if isinstance(v, float) and not math.isfinite(v):
                    raise ValueError(
                        f"Non-finite float in object column {col!r} at index {idx!r}."
                    )


def _type_has_timezone(typ: Any) -> bool:
    return bool(getattr(typ, "timezone", False))


def _cell_to_date(value: Any) -> date:
    if isinstance(value, pd.Timestamp):
        return to_date(value.to_pydatetime())
    if isinstance(value, np.datetime64):
        return to_date(pd.Timestamp(value).to_pydatetime())
    return to_date(value)


def _cell_to_naive_datetime(value: Any) -> datetime:
    """TIMESTAMP WITHOUT TIME ZONE (naive Python datetimes for the driver)."""
    if isinstance(value, str):
        try:
            return to_datetime(value)
        except ValueError:
            parsed = pd.to_datetime(value, errors="raise")
            p = parsed.to_pydatetime()
            if p.tzinfo is not None:
                p = p.astimezone(timezone.utc).replace(tzinfo=None)
            return p
    if isinstance(value, pd.Timestamp):
        p = value.to_pydatetime()
    elif isinstance(value, np.datetime64):
        p = pd.Timestamp(value).to_pydatetime()
    elif isinstance(value, datetime):
        p = value
    elif isinstance(value, date):
        return to_datetime(value)
    else:
        raise TypeError(
            f"Expected str, date, datetime, pandas.Timestamp, or numpy.datetime64; "
            f"got {type(value).__name__}"
        )
    if p.tzinfo is not None:
        p = p.astimezone(timezone.utc).replace(tzinfo=None)
    return p


def _cell_to_utc_aware_datetime(value: Any) -> datetime:
    """TIMESTAMPTZ (UTC-aware Python datetimes)."""
    if isinstance(value, str):
        try:
            d = to_date(value)
            return datetime.combine(d, time.min, tzinfo=timezone.utc)
        except ValueError:
            try:
                dt = to_datetime(value)
            except ValueError:
                parsed = pd.to_datetime(value, utc=True, errors="raise")
                return parsed.to_pydatetime()
            if dt.tzinfo is None:
                return dt.replace(tzinfo=timezone.utc)
            return dt.astimezone(timezone.utc)
    if isinstance(value, pd.Timestamp):
        p = value.to_pydatetime()
    elif isinstance(value, np.datetime64):
        p = pd.Timestamp(value).to_pydatetime()
    elif isinstance(value, datetime):
        p = value
    elif isinstance(value, date):
        return datetime.combine(value, time.min, tzinfo=timezone.utc)
    else:
        raise TypeError(
            f"Expected str, date, datetime, pandas.Timestamp, or numpy.datetime64; "
            f"got {type(value).__name__}"
        )
    if p.tzinfo is None:
        return p.replace(tzinfo=timezone.utc)
    return p.astimezone(timezone.utc)


def _coerce_dataframe_for_table(df: pd.DataFrame, column_metas: list[dict[str, Any]]) -> pd.DataFrame:
    """Normalize date/datetime columns using :mod:`quantithaca.utils.dateutils` where possible."""
    out = df.copy()
    meta_by_name = {m["name"]: m for m in column_metas}

    for c in out.columns:
        typ = meta_by_name[c]["type"]
        series = out[c]

        if isinstance(typ, Date):
            out[c] = [_cell_to_date(v) for v in series]
        elif isinstance(typ, DateTime) or isinstance(typ, TIMESTAMP):
            if _type_has_timezone(typ):
                out[c] = [_cell_to_utc_aware_datetime(v) for v in series]
            else:
                out[c] = [_cell_to_naive_datetime(v) for v in series]

    return out


def _sanitize_cell(value: Any) -> Any:
    if isinstance(value, pd.Timestamp):
        return value.to_pydatetime()
    if isinstance(value, np.datetime64):
        return pd.Timestamp(value).to_pydatetime()
    if isinstance(value, np.integer):
        return int(value)
    if isinstance(value, np.floating):
        return float(value)
    if isinstance(value, np.bool_):
        return bool(value)
    return value


def _records_from_dataframe(df: pd.DataFrame) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for rec in df.to_dict(orient="records"):
        rows.append({k: _sanitize_cell(v) for k, v in rec.items()})
    return rows


def _reflect_table(conn: Connection, schema: str, table: str) -> Table:
    md = MetaData()
    md.reflect(bind=conn, schema=schema, only=[table])
    key = f"{schema}.{table}"
    if key in md.tables:
        return md.tables[key]
    if len(md.tables) == 1:
        return next(iter(md.tables.values()))
    available = ", ".join(sorted(md.tables.keys()))
    raise ValueError(
        f"Table {schema!r}.{table!r} not found after reflection. Available: {available}"
    )


def sql_write(
    df: pd.DataFrame,
    *,
    table: str,
    schema: str,
    engine: Engine,
    flag: Literal["insert", "upsert"],
) -> None:
    """
    Write ``df`` to a PostgreSQL table using *engine*.

    * Column names must match the table exactly (order follows the database).
    * Missing values, NaT, NaN, and infinities are rejected.
    * ``DATE`` / ``TIMESTAMP`` / ``TIMESTAMPTZ`` columns are converted to Python
      ``date`` / ``datetime`` values. String cells use :func:`quantithaca.utils.dateutils.to_date`
      and :func:`quantithaca.utils.dateutils.to_datetime` (formats ``d/m/Y``, ``d.m.Y``,
      ``Y-m-d``, ``Ymd``). Timestamp strings that include a time or offset fall back to
      :func:`pandas.to_datetime` (UTC) for ``TIMESTAMPTZ`` only.
    * ``upsert`` uses ``ON CONFLICT`` on the primary key; non-key columns are
      updated from the new row. If the table has only a primary key and no other
      columns, conflicts are ignored (insert-or-skip).
    """
    if engine.dialect.name != "postgresql":
        raise NotImplementedError("sql_write only supports PostgreSQL engines.")
    if pg_insert is None:  # pragma: no cover
        raise RuntimeError("PostgreSQL dialect is not available.")

    insp = inspect(engine)
    if not insp.has_table(table, schema=schema):
        raise ValueError(f"Table {schema!r}.{table!r} does not exist.")

    column_metas = insp.get_columns(table, schema=schema)
    db_cols = [c["name"] for c in column_metas]
    df_cols = list(df.columns)

    if set(df_cols) != set(db_cols):
        raise ValueError(
            f"DataFrame columns must match table columns exactly. "
            f"Table: {sorted(db_cols)}. DataFrame: {sorted(df_cols)}."
        )

    df_ordered = df[db_cols].copy()
    _assert_no_missing_or_bad_values(df_ordered)
    df_ready = _coerce_dataframe_for_table(df_ordered, column_metas)
    _assert_no_missing_or_bad_values(df_ready)
    rows = _records_from_dataframe(df_ready)

    pk_info = insp.get_pk_constraint(table, schema=schema)
    pk_cols = list(pk_info.get("constrained_columns") or [])

    print(f"{flag} {df.shape[0]} rows into {schema}.{table}")

    with engine.begin() as conn:
        tbl = _reflect_table(conn, schema, table)
        if flag == "insert":
            conn.execute(insert(tbl), rows)
            return
        elif flag == "upsert":
            if not pk_cols:
                raise ValueError("upsert requires a primary key on the table.")
            update_cols = [c for c in db_cols if c not in pk_cols]
            ins = pg_insert(tbl)
            if not update_cols:
                stmt = ins.on_conflict_do_nothing(index_elements=pk_cols)
            else:
                stmt = ins.on_conflict_do_update(
                    index_elements=pk_cols,
                    set_={c: getattr(ins.excluded, c) for c in update_cols},
                )
            conn.execute(stmt, rows)
        else:
            raise ValueError(f"Flag {flag} not known.")
