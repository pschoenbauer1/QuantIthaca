
import pandas as pd
import os
import re

from quantithaca.db import neon_rw_engine, neon_ro_engine
from quantithaca.sql_write import sql_write
from quantithaca.logging import logger
from quantithaca.utils import dateutils as du

RLB_CASHFLOW_DIR = r"C:\Users\phili\OneDrive\Konten\Raiffeisen\Transactions_csv"

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

def read_rlb_cashflows_sql():
    data = pd.read_sql("SELECT * FROM finance.rlb_cashflows ORDER BY ref_date", neon_ro_engine, parse_dates=["ref_date"])
    return data

def rlb_cashflows_to_neondb(data_new):
    assert set(data_new.columns) == {"account_name", "ref_date", "group_idx", "description", "ref_date", "amount", "ccy", "timestamp_cet"}

    data = read_rlb_cashflows_sql()
    data = data.sort_values("ref_date")
    data_new = data_new.sort_values("ref_date")
    max_date_old = max([du.to_date(d) for d in data["ref_date"].tolist()])

    data_new = data_new[data_new["ref_date"].apply(du.to_date) > max_date_old].copy()

    logger.info(f"Found {data.shape[0]} existing rows.")
    logger.info(f"Adding {data_new.shape[0]} rows.")

    sql_write(
        data_new,
        schema="finance",
        table="rlb_cashflows",
        engine=neon_rw_engine,
        flag="insert")


def rlb_cashflows_from_csv_to_neondb():
    datanew = read_rlb_cashflows_csv()
    rlb_cashflows_to_neondb(datanew)

def read_rlb_pnl():
    query = """
        SELECT *
        FROM finance.pnl
        WHERE account_name LIKE %(pattern)s
        """

    data = pd.read_sql(query, neon_rw_engine, params={"pattern": "RLB-CURRENT%"})
    return data

def rlb_write_pnl():
    pnlnew = pd.read_excel(f"{RLB_CASHFLOW_DIR}/allocations.xlsx")
    pnlnew = pnlnew[(pnlnew["bu"]!="") & (~pnlnew["bu"].isna())].copy().reset_index(drop=True)
    pnlnew = pnlnew[["account_name", "ref_date", "group_idx", "amount", "bu", "b_year", "b_month", "category"]].copy()
    pnlold = read_rlb_pnl()
    if not pnlold.empty:
        max_date_old = max([du.to_date(d) for d in pnlold["ref_date"].tolist()])
        pnlnew = pnlnew[pnlnew["ref_date"]>max_date_old]

    logger.info(f"Found {pnlold.shape[0]} existing rows.")
    logger.info(f"Adding {pnlnew.shape[0]} rows.")
    
    sql_write(
        pnlnew,
        schema="finance",
        table="pnl",
        engine=neon_rw_engine,
        flag="insert")



if __name__ == "__main__":
    # rlb_cashflows_from_csv_to_neondb()
    rlb_write_pnl()
