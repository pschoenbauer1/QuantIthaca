
import calendar
import json
import os
import re
from datetime import date, datetime
from pathlib import Path
from typing import Literal

import pandas as pd
from sqlalchemy import text

from quantithaca.db import load_dotenv_optional, neon_rw_engine, neon_ro_engine
from quantithaca.logging import logger
from quantithaca.sql_write import sql_write
from quantithaca.utils import dateutils as du

load_dotenv_optional()

RLB_CASHFLOW_DIR = os.getenv("RLB_CASHFLOW_DIR") or r"C:\Users\phili\OneDrive\Konten\Raiffeisen\Transactions_csv"
ALLOCATION_CONFIG_PATH = Path(__file__).resolve().parent / "configs" / "allocation.json"

RLB_CASHFLOW_KEY_COLUMNS = ["account_name", "ref_date", "group_idx"]

RLB_CASHFLOW_COLUMNS = {
    "account_name",
    "ref_date",
    "group_idx",
    "description",
    "amount",
    "ccy",
    "timestamp_cet",
}

RLB_PNL_COLUMNS = {
    "account_name",
    "ref_date",
    "group_idx",
    "amount",
    "ccy",
    "bu",
    "b_year",
    "b_month",
    "category",
}

PnlWriteMode = Literal["insert", "insert_incremental", "upsert"]
PNL_KEY_COLUMNS = ["account_name", "ref_date", "group_idx"]

RLB_ALLOC_REGEX = {
    "Kuebeck13": r".*?K(?:.|..|...)beckgasse 12.*?13.*?|.*?A564166830.*?|.*?SEPA-Lastschrift A564166830 A-1030 Wien, Rennweg 70/3/9 A-1030 Wien, K uebeckgasse 12/13.*?",
    "Erdberg36": r".*?Erdberg.*?8.*?1130740.*?",
    "Erdberg47": r".*?Erdberg.*?8.*?1130850.*?",
    "Erdberg48": r".*?Erdberg.*?8.*?1130860.*?",
    "Rennweg9": r".*?Rennweg.*?7(?:\s?)0.3.9|VORSCHR:.*?-0309-01|.*?A560545218.*?"
}


def read_rlb_cashflows_csv(directory=RLB_CASHFLOW_DIR):
    files = os.listdir(directory)

    datas = []

    for file in files:
        key_len = len("RLB-CURRENT-0")
        if len(file) < key_len or (file[:key_len] != "RLB-CURRENT-0" and file[:key_len] != "RLB-CURRENT-1"):
            continue

        account = file[:key_len]

        filename = f"{directory}/{file}"
        data = pd.read_csv(filename, sep=";", header=None, decimal=",")
        data.columns = ["date", "description", "ref_date", "amount", "ccy", "timestamp_cet"]
        data = data.drop(columns=["date"])

        data["account_name"] = account
        data = data.groupby(["account_name", "ref_date"]).apply(
            lambda df: df.reset_index(drop=True).reset_index(names=["group_idx"])
        ).reset_index().drop(columns='level_2')
        datas.append(data)

    data = pd.concat(datas)

    return data


def load_allocation_config(path=ALLOCATION_CONFIG_PATH):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def load_allocation_counterparties(path=ALLOCATION_CONFIG_PATH):
    return load_allocation_config(path)["counterparties"]


def load_allocation_bu_rules(path=ALLOCATION_CONFIG_PATH):
    return load_allocation_config(path)["bu_rules"]


def load_allocation_category_rules(path=ALLOCATION_CONFIG_PATH):
    return load_allocation_config(path)["category_rules"]


def read_rlb_cashflows_sql():
    data = pd.read_sql(
        "SELECT * FROM finance.rlb_cashflows ORDER BY ref_date",
        neon_ro_engine,
        parse_dates=["ref_date"],
    )
    return data


def read_rlb_pnl():
    query = """
        SELECT *
        FROM finance.pnl
        WHERE account_name LIKE %(pattern)s
        """

    data = pd.read_sql(query, neon_rw_engine, params={"pattern": "RLB-CURRENT%"})
    return data


def read_rlb_pnl_excel(directory=RLB_CASHFLOW_DIR):
    return pd.read_excel(f"{directory}/allocations.xlsx")


def compute_new_rows(data_new, data_old, date_col="ref_date"):
    if data_old.empty:
        return data_new.copy()

    max_date_old = max(du.to_date(d) for d in data_old[date_col].tolist())
    return data_new[data_new[date_col].apply(du.to_date) > max_date_old].copy()


def compute_rlb_pnl(pnl_raw):
    pnl = pnl_raw[(pnl_raw["bu"] != "") & (~pnl_raw["bu"].isna())].copy()
    return pnl[list(RLB_PNL_COLUMNS)].reset_index(drop=True)


def _is_populated(val) -> bool:
    if val is None:
        return False
    if isinstance(val, float) and pd.isna(val):
        return False
    if isinstance(val, str) and val == "":
        return False
    return True


def _empty_allocation() -> dict:
    return {"cp": None, "bu": None, "category": None, "b_year": None, "b_month": None, "error": None}


def allocate_booking_period(ref_date) -> tuple[int, int]:
    """Payments in the last 5 days of a month are booked to the next month."""
    d = du.to_date(ref_date)
    last_day = calendar.monthrange(d.year, d.month)[1]
    if d.day > last_day - 5:
        if d.month == 12:
            return d.year + 1, 1
        return d.year, d.month + 1
    return d.year, d.month


def apply_allocation_rules(
    cashflows: pd.DataFrame,
    allocation: dict,
    description_col: str = "description",
) -> pd.DataFrame:

    df = cashflows.copy()
    if "is_private" not in df.columns:
        df["is_private"] = False

    counterparties = allocation["counterparties"]
    bu_rules = allocation["bu_rules"]
    category_rules = allocation["category_rules"]
    ignore_patterns = [
        re.compile(pattern, re.IGNORECASE)
        for pattern in allocation.get("patterns_ignore", [])
    ]

    iban_to_cp = {}
    cp_patterns = []

    for cp in counterparties:
        cp_id = cp["counterparty_id"]

        for account in cp.get("accounts", []):
            iban = account.get("iban")
            if iban:
                iban_to_cp[iban.replace(" ", "").upper()] = cp_id

        for pattern in cp.get("patterns", []):
            cp_patterns.append(
                (re.compile(pattern, re.IGNORECASE), cp_id)
            )

    bu_patterns = [
        (re.compile(pattern, re.IGNORECASE), rule["bu"])
        for rule in bu_rules
        for pattern in rule["patterns"]
    ]

    category_patterns = [
        (re.compile(pattern, re.IGNORECASE), rule["category"])
        for rule in category_rules
        for pattern in rule["patterns"]
    ]

    iban_regex = re.compile(r"\b[A-Z]{2}\d{2}[A-Z0-9]{11,30}\b")

    def classify(text: str, is_private: bool, ref_date) -> dict:
        if is_private:
            return _empty_allocation()

        text = "" if pd.isna(text) else str(text)

        cps = set()
        for iban in iban_regex.findall(text):
            cp_id = iban_to_cp.get(iban.upper())
            if cp_id:
                cps.add(cp_id)

        for regex, cp_id in cp_patterns:
            if regex.search(text):
                cps.add(cp_id)

        if len(cps) == 0:
            for regex in ignore_patterns:
                if regex.search(text):
                    return _empty_allocation()
            return {**_empty_allocation(), "error": f"No counterparty found for text: {text}"}
        if len(cps) > 1:
            return {
                **_empty_allocation(),
                "error": f"Multiple counterparties found for text: {text}: {cps}",
            }
        cp = next(iter(cps))

        bus = set()
        for regex, bu_name in bu_patterns:
            if regex.search(text):
                bus.add(bu_name)

        if len(bus) == 0:
            return {**_empty_allocation(), "error": f"No BU found for text: {text}"}
        if len(bus) > 1:
            return {
                **_empty_allocation(),
                "error": f"Multiple BUs found for text: {text}: {bus}",
            }
        bu = next(iter(bus))

        categories = set()
        for regex, category_name in category_patterns:
            if regex.search(text):
                categories.add(category_name)

        if len(categories) == 0:
            return {**_empty_allocation(), "error": f"No category found for text: {text}"}
        if len(categories) > 1:
            return {
                **_empty_allocation(),
                "error": f"Multiple categories found for text: {text}: {categories}",
            }
        category = next(iter(categories))
        b_year, b_month = allocate_booking_period(ref_date)

        return {
            "cp": cp,
            "bu": bu,
            "category": category,
            "b_year": b_year,
            "b_month": b_month,
            "error": None,
        }

    classified = []
    for idx, row in df.iterrows():
        classified.append(
            classify(row[description_col], bool(row["is_private"]), row["ref_date"])
        )

    allocation_df = pd.DataFrame(classified, index=df.index)
    df[["cp", "bu", "category", "b_year", "b_month", "error"]] = allocation_df[
        ["cp", "bu", "category", "b_year", "b_month", "error"]
    ]

    error_rows = df[df["error"].apply(_is_populated) & ~df["is_private"].astype(bool)]
    for idx, row in error_rows.iterrows():
        logger.error(
            f"row {idx} (account_name={row['account_name']}, ref_date={row['ref_date']}, "
            f"group_idx={row['group_idx']}): {row['error']}"
        )

    return df


def enrich_cashflows(allocation_path=ALLOCATION_CONFIG_PATH):
    cashflows = read_rlb_cashflows_sql()
    allocation = load_allocation_config(allocation_path)
    return apply_allocation_rules(cashflows, allocation)


def save_rlb_cashflows_is_private(original: pd.DataFrame, edited: pd.DataFrame) -> int:
    for col in RLB_CASHFLOW_KEY_COLUMNS + ["is_private", "error"]:
        if col not in original.columns:
            raise ValueError(f"Missing column in original data: {col}")
    if "is_private" not in edited.columns:
        raise ValueError("Missing is_private column in edited data.")

    if len(original) != len(edited):
        raise ValueError("Row count changed; save aborted.")

    orig = original.reset_index(drop=True)
    edit = edited.reset_index(drop=True)

    changed_mask = orig["is_private"].astype(bool) != edit["is_private"].astype(bool)
    if not changed_mask.any():
        return 0

    changed = orig.loc[changed_mask]
    no_error = changed[~changed["error"].apply(_is_populated)]
    if not no_error.empty:
        examples = [
            f"account_name={row['account_name']}, ref_date={row['ref_date']}, group_idx={row['group_idx']}"
            for _, row in no_error.head(5).iterrows()
        ]
        raise ValueError(
            f"Cannot update is_private for {len(no_error)} row(s) without allocation error. "
            f"Examples: {'; '.join(examples)}"
        )

    if neon_rw_engine is None:
        raise ValueError("Database write engine is not configured.")

    stmt = text(
        """
        UPDATE finance.rlb_cashflows
        SET is_private = :is_private
        WHERE account_name = :account_name
          AND ref_date = :ref_date
          AND group_idx = :group_idx
        """
    )

    updated = 0
    with neon_rw_engine.begin() as conn:
        for idx in changed.index:
            row = edit.loc[idx]
            result = conn.execute(
                stmt,
                {
                    "is_private": bool(row["is_private"]),
                    "account_name": row["account_name"],
                    "ref_date": row["ref_date"],
                    "group_idx": int(row["group_idx"]),
                },
            )
            updated += result.rowcount

    logger.info(f"Updated is_private for {updated} row(s) in finance.rlb_cashflows.")
    return updated


def _prepare_pnl_df(data: pd.DataFrame) -> pd.DataFrame:
    data = data[list(RLB_PNL_COLUMNS)]
    if set(data.columns) != RLB_PNL_COLUMNS:
        raise ValueError(
            f"DataFrame columns must be {sorted(RLB_PNL_COLUMNS)}; got {sorted(data.columns)}."
        )
    if data.empty:
        raise ValueError("P&L DataFrame is empty.")

    out = data[list(RLB_PNL_COLUMNS)].copy()
    out["ref_date"] = out["ref_date"].apply(du.to_date)
    out["group_idx"] = out["group_idx"].astype(int)
    out["b_year"] = out["b_year"].astype(int)
    out["b_month"] = out["b_month"].astype(int)
    out["amount"] = out["amount"].astype(float)
    out["ccy"] = out["ccy"].astype(str)
    out["bu"] = out["bu"].astype(str)
    out["category"] = out["category"].astype(str)
    out["account_name"] = out["account_name"].astype(str)
    return out


def _sql_literal(value) -> str:
    if value is None or (isinstance(value, float) and pd.isna(value)):
        return "NULL"
    if isinstance(value, bool):
        return "TRUE" if value else "FALSE"
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, (date, datetime)):
        return f"'{value.isoformat()}'"
    text = str(value).replace("'", "''")
    return f"'{text}'"


def _pnl_replace_sql(incoming_idx: pd.DataFrame, diff: pd.DataFrame) -> str:
    statements = []
    for key in diff.index.unique():
        row = incoming_idx.loc[key]
        if isinstance(row, pd.DataFrame):
            row = row.squeeze()

        diff_row = diff.loc[key]
        if isinstance(diff_row, pd.DataFrame):
            diff_row = diff_row.iloc[0]

        changed_cols = sorted(
            col
            for col in diff.columns.get_level_values(0).unique()
            if (col, "self") in diff_row.index and pd.notna(diff_row[(col, "self")])
        )
        account_name, ref_date, group_idx = key
        sets = ", ".join(f"{col} = {_sql_literal(row[col])}" for col in changed_cols)
        where = (
            f"account_name = {_sql_literal(account_name)} AND "
            f"ref_date = {_sql_literal(ref_date)} AND "
            f"group_idx = {_sql_literal(group_idx)}"
        )
        statements.append(f"UPDATE finance.pnl SET {sets} WHERE {where};")
    return "\n".join(statements)


def cashflows_to_pnl(enriched: pd.DataFrame) -> pd.DataFrame:
    ok = (
        ~enriched["error"].apply(_is_populated)
        & enriched["bu"].apply(_is_populated)
        & enriched["category"].apply(_is_populated)
        & enriched["b_year"].apply(_is_populated)
        & enriched["b_month"].apply(_is_populated)
    )
    if not ok.any():
        raise ValueError("No successfully allocated cashflow rows to write as P&L.")

    return enriched.loc[ok]


def write_pnl(data: pd.DataFrame, mode: PnlWriteMode = "insert") -> int:
    if neon_rw_engine is None:
        raise ValueError("Database write engine is not configured.")

    df = data.copy()
    if "is_private" in df.columns:
        df = df[~df["is_private"].astype(bool)].copy()

    if df.empty:
        raise ValueError("No non-private rows to write as P&L.")

    missing_cols = RLB_PNL_COLUMNS - set(df.columns)
    if missing_cols:
        raise ValueError(f"Missing P&L columns: {sorted(missing_cols)}")

    df = df[list(RLB_PNL_COLUMNS)]

    incoming = _prepare_pnl_df(df)
    existing = read_rlb_pnl()
    if not existing.empty:
        existing = _prepare_pnl_df(existing)

    incoming_idx = incoming.set_index(PNL_KEY_COLUMNS).sort_index()
    existing_idx = (
        existing.set_index(PNL_KEY_COLUMNS).sort_index()
        if not existing.empty
        else incoming_idx.iloc[0:0]
    )

    if mode == "insert":
        overlap = incoming_idx.index.intersection(existing_idx.index)
        if not overlap.empty:
            examples = [str(key) for key in list(overlap[:5])]
            raise ValueError(
                f"{len(overlap)} row(s) already exist in finance.pnl. Examples: {'; '.join(examples)}"
            )
        to_write = incoming

    elif mode == "insert_incremental":
        common = incoming_idx.index.intersection(existing_idx.index)
        if not common.empty:
            diff = incoming_idx.loc[common].compare(existing_idx.loc[common])
            if not diff.empty:
                diff_keys = diff.index.unique()
                replace_sql = _pnl_replace_sql(incoming_idx, diff)
                print(replace_sql)
                logger.error(diff)
                raise ValueError(
                    f"{len(diff_keys)} overlapping P&L row(s) do not match existing data; aborting.\n\n"
                    f"Replacement SQL (not executed):\n{replace_sql}"
                )
        new_keys = incoming_idx.index.difference(existing_idx.index)
        to_write = incoming_idx.loc[new_keys].reset_index()
        if to_write.empty:
            logger.info("No new P&L rows to insert.")
            return 0

    elif mode == "upsert":
        to_write = incoming

    else:
        raise ValueError(f"Unknown P&L write mode: {mode!r}")

    logger.info(f"{mode} {to_write.shape[0]} rows into finance.pnl.")
    print(to_write)
    sql_write(
        to_write,
        schema="finance",
        table="pnl",
        engine=neon_rw_engine,
        flag="upsert" if mode == "upsert" else "insert",
    )
    return len(to_write)


def write_rlb_cashflows(data):
    assert set(data.columns) == RLB_CASHFLOW_COLUMNS

    logger.info(f"Adding {data.shape[0]} rows to finance.rlb_cashflows.")

    sql_write(
        data,
        schema="finance",
        table="rlb_cashflows",
        engine=neon_rw_engine,
        flag="insert",
    )

def rlb_cashflows_from_csv_to_neondb(directory=RLB_CASHFLOW_DIR):
    data_new = read_rlb_cashflows_csv(directory)
    data_old = read_rlb_cashflows_sql()
    data_new = compute_new_rows(data_new, data_old)

    logger.info(f"Found {data_old.shape[0]} existing rows.")

    if data_new.empty:
        logger.info("No new cashflow rows to add.")
        return

    write_rlb_cashflows(data_new)

if __name__ == "__main__":
    # rlb_cashflows_from_csv_to_neondb()
    # rlb_pnl_from_excel_to_neondb()
    data = enrich_cashflows()
    print(data)
