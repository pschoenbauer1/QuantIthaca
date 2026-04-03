
import pandas as pd
import os

from quantithaca.db import neon_rw_engine, neon_ro_engine
from quantithaca.sql_write import sql_write
from quantithaca.logging import logger
from quantithaca.utils import dateutils as du

RLB_CASHFLOW_DIR = r"C:\Users\phili\OneDrive\Konten\Raiffeisen\Transactions_csv"

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
    min_date_new = data_new["ref_date"].min()
    max_date_old = data["ref_date"].max()

    logger.info(f"Found {data.shape[0]} existing rows.")
    logger.info(f"Adding {data_new.shape[0]} rows.")

    if not data.empty and du.to_date(min_date_new) <= du.to_date(max_date_old):
        raise ValueError(f"Aborting: existing data until {max_date_old}, attempted insert from {min_date_new}")
    
    sql_write(
        data_new,
        schema="finance",
        table="rlb_cashflows",
        engine=neon_rw_engine,
        flag="insert")


def rlb_cashflows_from_csv_to_neondb():
    datanew = read_rlb_cashflows_csv()
    rlb_cashflows_to_neondb(datanew)

if __name__ == "__main__":
    rlb_cashflows_from_csv_to_neondb()


