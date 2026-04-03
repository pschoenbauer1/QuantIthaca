import os

import pytest

from quantithaca.db import neon_ro_engine, neon_rw_engine

import pandas as pd

def test_neon_db_rw():
    data = pd.read_sql("SELECT * FROM public.test_table", neon_rw_engine)
    assert len(data) > 0

    
def test_neon_db_ro():
    data = pd.read_sql("SELECT * FROM public.test_table", neon_ro_engine)
    assert len(data) > 0