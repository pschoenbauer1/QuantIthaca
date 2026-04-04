

import pandas as pd
import os
import re

from quantithaca.db import neon_rw_engine, neon_ro_engine
from quantithaca.sql_write import sql_write
from quantithaca.logging import logger
from quantithaca.utils import dateutils as du

TAX_DIRECTORY = r"C:\Users\phili\OneDrive\Steuererklaerungen"


def read_pnl():
    query = "SELECT * FROM finance.pnl"

    data = pd.read_sql(query, neon_rw_engine, parse_dates=["ref_date"])
    return data

def read_bu():
    query = "SELECT * FROM finance.bu"
    data = pd.read_sql(query, neon_rw_engine)
    return data

def read_pnl_categories():
    query = "SELECT * FROM finance.pnl_categories"
    data = pd.read_sql(query, neon_rw_engine)
    return data

def write_pnl():
    file = f"{TAX_DIRECTORY}/pnl.xlsx"
    data_new = pd.read_excel(file)
    data_old = read_pnl()

    data_new = data_new[data_old.columns]

    key = ["account_name", "ref_date", "group_idx"]
    data_new = data_new.set_index(key).sort_index()
    data_old = data_old.set_index(key).sort_index()
    new_keys = data_new.index.difference(data_old.index)
    common_keys = data_new.index.intersection(data_old.index)
    diff = data_new.loc[common_keys].compare(data_old.loc[common_keys])    
    if not diff.empty:
        logger.warning(diff)
        raise ValueError("Stopping due to inconsistent data.")

    data_new = data_new.loc[new_keys].reset_index()

    sql_write(
        data_new,
        table="pnl",
        schema="finance",
        engine=neon_rw_engine,
        flag="insert")

if __name__ == "__main__":
    write_pnl()


