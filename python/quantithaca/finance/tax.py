

import pandas as pd
import numpy as np
import os
import re

from quantithaca.db import neon_rw_engine, neon_ro_engine
from quantithaca.sql_write import sql_write
from quantithaca.logging import logger
from quantithaca.utils import dateutils as du
from quantithaca.utils import pdf_tables
from quantithaca.finance.pnl import read_pnl, read_bu, read_pnl_categories, TAX_DIRECTORY


def read_taxrates_aut(year:int):
    data = pd.read_excel(f"{TAX_DIRECTORY}/pnl.xlsx", sheet_name="aut_tax_brackets")
    data = data[data["year"]==year]
    return data

def compute_pnl_aut(year:int):
    pnl = read_pnl()
    bu = read_bu().rename(columns={"aut_tax_relevant":"bu_aut_tax_relevant"})
    pnl_cat = read_pnl_categories().rename(columns={"aut_tax_relevant":"cat_aut_tax_relevant"})

    pnl = pd.merge(pnl, bu, on = "bu")
    pnl = pd.merge(pnl, pnl_cat, on = "category")

    pnl = pnl[pnl["b_year"] == year].copy()
    pnl = pnl[pnl["bu_aut_tax_relevant"]].copy()
    pnl = pnl[pnl["cat_aut_tax_relevant"]].copy()
    pnl = pnl[["account_name",
               "ref_date",
               "group_idx",
               "amount",
               "bu",
               "b_year",
               "b_month",
               "category",
               "ccy", 
               "bu_aut_tax_relevant",
               "cat_aut_tax_relevant",
               "is_income"]]

    logger.info(f"Processing {pnl.shape[0]} pnl lines for AUT tax year {year}.")

    pnl_grouped1 = pnl.groupby(["bu", "category", "ccy"])["amount"].sum().reset_index()
    pnl_grouped2 = pnl_grouped1.groupby(["bu", "ccy"])["amount"].sum().reset_index()
    pnl_sum = pnl_grouped2.groupby("ccy")["amount"].sum()

    print(pnl)
    print(pnl_grouped1)
    print(pnl_grouped2)
    print(pnl_sum)

    assert pnl_sum.shape[0] == 1
    assert pnl_sum.index[0] == "EUR"

    pnl_total = pnl_sum.iloc[0]

    return pnl, pnl_total


def compute_tax_aut(year:int):
    pnl, pnl_total  = compute_pnl_aut(year)
    tax  = read_taxrates_aut(year)

    tax = tax.sort_values("limit")
    tax["lower_limit"] = tax["limit"].shift(1).fillna(0.0)
    tax["attr"] = np.minimum(np.maximum(pnl_total - tax["lower_limit"], 0.0), tax["limit"] - tax["lower_limit"]).astype(float)
    tax["tax"] = tax["attr"] * tax["tax_rate"]
    tax["limit"] = tax["limit"].where(tax["limit"]<1e+8, np.nan)

    return {"pnl": pnl, "tax": tax}

def tax_aut_write_pdf(year:int):
    tables = compute_tax_aut(2024) 

    pnl = tables["pnl"]
    pnl = pnl[["account_name",
               "ref_date",
               # "group_idx",
               "amount",
               "ccy",
               "bu",
               "b_month",
               "category",
               ]].copy()
    pnl = pnl.sort_values(["bu", "ref_date", "category"])
    pnl = pnl.set_index(["category", "b_month"])

    table_tax = tables["tax"].drop(columns={"year"})[["lower_limit", "limit", "tax_rate", "attr", "tax"]].set_index("lower_limit")
    table_overview = pnl.groupby(["bu", "category", "ccy"])[["amount"]].sum().reset_index().set_index(["bu", "category"])[["amount", "ccy"]]
    tables_bu = {f"P&L {k}": v.drop(columns=["bu"]) for k, v in pnl.groupby('bu')}
    
    tax_table_name = f"Tax AUT {year}"
    tables_pdf = {tax_table_name: table_tax, "P&L Overview": table_overview, **tables_bu}
    
    
    col_specs_ = {
        "group_idx": {"agg":None},
        "amount": {"float_format": "{:,.2f}"}}
    col_specs = {k: col_specs_ for k in tables_bu}
    col_specs[tax_table_name] = {"year": {"agg": None},
                        "tax_rate": {"agg": None, "float_format":"{:,.0%}"},
                        "limit": {"agg": None, "float_format":"{:,.2f}"},
                        "lower_limit": {"agg": None, "float_format":"{:,.0f}"},
                        "attr": {"float_format":"{:,.0f}"},
                        "tax": {"float_format":"{:,.0f}"}}
    col_specs["P&L Overview"] = {"amount": {"float_format": "{:,.2f}"}}

    pdf_tables.dataframes_to_pdf(
        tables_pdf, 
        f"{TAX_DIRECTORY}/reports/tax_aut_2024.pdf",
        #landscape=True,
        section_column_specs=col_specs,
        overwrite=True)
    


if __name__ == "__main__":
    tax = tax_aut_write_pdf(2024) 
    
    # pdf_tables.dataframes_to_pdf(tax, f"{TAX_DIRECTORY}/reports/tax_aut_2024.pdf")