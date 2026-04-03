import numpy as np
import pandas as pd
import pytest
from sqlalchemy import create_engine

from quantithaca.sql_write import (
    _assert_no_missing_or_bad_values,
    _coerce_dataframe_for_table,
    sql_write,
)


def test_assert_rejects_empty():
    with pytest.raises(ValueError, match="empty"):
        _assert_no_missing_or_bad_values(pd.DataFrame({"a": []}))


def test_assert_rejects_nan():
    df = pd.DataFrame({"a": [1.0, np.nan]})
    with pytest.raises(ValueError, match="Null"):
        _assert_no_missing_or_bad_values(df)


def test_assert_rejects_nat():
    df = pd.DataFrame({"t": pd.to_datetime(["2020-01-01", pd.NaT])})
    with pytest.raises(ValueError, match="Null"):
        _assert_no_missing_or_bad_values(df)


def test_assert_rejects_inf():
    df = pd.DataFrame({"a": [1.0, np.inf]})
    with pytest.raises(ValueError, match="Non-finite"):
        _assert_no_missing_or_bad_values(df)


def test_assert_rejects_object_float_inf():
    df = pd.DataFrame({"a": [1.0, float("inf")]}, dtype=object)
    with pytest.raises(ValueError, match="Non-finite float"):
        _assert_no_missing_or_bad_values(df)


def test_coerce_date_and_timestamptz():
    from sqlalchemy import Date, DateTime
    from sqlalchemy.dialects.postgresql import TIMESTAMP

    metas = [
        {"name": "d", "type": Date()},
        {"name": "ts", "type": DateTime(timezone=False)},
        {"name": "tstz", "type": TIMESTAMP(timezone=True)},
    ]
    df = pd.DataFrame(
        {
            "d": ["2024-06-01"],
            "ts": [pd.Timestamp("2024-06-01 12:00:00")],
            "tstz": [pd.Timestamp("2024-06-01 12:00:00+01:00")],
        }
    )
    out = _coerce_dataframe_for_table(df, metas)
    assert out["d"].iloc[0].year == 2024
    assert out["ts"].iloc[0].tzinfo is None
    assert out["tstz"].iloc[0].tzinfo is not None


def test_sql_write_non_postgres_engine():
    engine = create_engine("sqlite:///:memory:")
    df = pd.DataFrame({"a": [1]})
    with pytest.raises(NotImplementedError, match="PostgreSQL"):
        sql_write(df, table="t", schema="main", engine=engine, flag="insert")
