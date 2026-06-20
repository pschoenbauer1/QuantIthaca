"""Postgres integration tests for ``sql_write`` (requires ``TEST_DATABASE_URL``)."""

from __future__ import annotations

import uuid
from datetime import date, datetime, timezone

import pandas as pd
import pytest
from sqlalchemy import text

from quantithaca.sql_write import sql_write

pytestmark = pytest.mark.integration


@pytest.fixture
def sql_write_table(pg_engine, qi_test_schema: str):
    table = f"sw_{uuid.uuid4().hex[:12]}"
    ddl = f"""
    CREATE TABLE "{qi_test_schema}"."{table}" (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        d DATE NOT NULL,
        ts TIMESTAMP WITHOUT TIME ZONE NOT NULL,
        tstz TIMESTAMPTZ NOT NULL
    )
    """
    with pg_engine.begin() as conn:
        conn.execute(text(ddl))
    try:
        yield table
    finally:
        with pg_engine.begin() as conn:
            conn.execute(text(f'DROP TABLE IF EXISTS "{qi_test_schema}"."{table}" CASCADE'))


def test_sql_write_insert(pg_engine, qi_test_schema: str, sql_write_table: str):
    df = pd.DataFrame(
        {
            "id": [1],
            "name": ["alpha"],
            "d": ["2024-06-01"],
            "ts": ["2024-06-01 12:00:00"],
            "tstz": ["2024-06-01 11:00:00+01:00"],
        }
    )
    sql_write(
        df,
        table=sql_write_table,
        schema=qi_test_schema,
        engine=pg_engine,
        flag="insert",
    )
    out = pd.read_sql(
        text(f'SELECT id, name, d, ts, tstz FROM "{qi_test_schema}"."{sql_write_table}" ORDER BY id'),
        pg_engine,
    )
    assert len(out) == 1
    assert out["name"].iloc[0] == "alpha"
    assert str(out["d"].iloc[0]) == "2024-06-01"


def test_sql_write_upsert_updates_row(pg_engine, qi_test_schema: str, sql_write_table: str):
    df1 = pd.DataFrame(
        {
            "id": [1],
            "name": ["first"],
            "d": [date(2024, 1, 1)],
            "ts": [datetime(2024, 1, 1, 8, 0, 0)],
            "tstz": [datetime(2024, 1, 1, 8, 0, 0, tzinfo=timezone.utc)],
        }
    )
    sql_write(
        df1,
        table=sql_write_table,
        schema=qi_test_schema,
        engine=pg_engine,
        flag="insert",
    )
    df2 = pd.DataFrame(
        {
            "id": [1],
            "name": ["second"],
            "d": [date(2024, 2, 1)],
            "ts": [datetime(2024, 2, 1, 9, 0, 0)],
            "tstz": [datetime(2024, 2, 1, 9, 0, 0, tzinfo=timezone.utc)],
        }
    )
    sql_write(
        df2,
        table=sql_write_table,
        schema=qi_test_schema,
        engine=pg_engine,
        flag="upsert",
    )
    out = pd.read_sql(
        text(f'SELECT name FROM "{qi_test_schema}"."{sql_write_table}" WHERE id = 1'),
        pg_engine,
    )
    assert out["name"].iloc[0] == "second"
