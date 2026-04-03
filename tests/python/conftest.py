"""Pytest fixtures (Postgres integration tests)."""

from __future__ import annotations

import os

import pytest
from sqlalchemy import create_engine, text
from sqlalchemy.engine import Engine


def _test_database_url() -> str:
    return (
        os.environ.get("TEST_DATABASE_URL", "").strip()
        or os.environ.get("QUANTITHACA_TEST_DATABASE_URL", "").strip()
    )


@pytest.fixture(scope="session")
def pg_engine() -> Engine:
    url = _test_database_url()
    if not url:
        pytest.skip(
            "Set TEST_DATABASE_URL for Postgres integration tests "
            "(see docker/docker-compose.postgres-test.yml)."
        )
    engine = create_engine(url, pool_pre_ping=True)
    try:
        with engine.connect() as conn:
            conn.execute(text("SELECT 1"))
    except OSError as exc:
        engine.dispose()
        pytest.skip(f"Postgres not reachable: {exc}")
    except Exception as exc:  # noqa: BLE001 — surface driver/auth errors as skip
        engine.dispose()
        pytest.skip(f"Postgres not usable: {exc}")
    yield engine
    engine.dispose()


@pytest.fixture(scope="session")
def qi_test_schema(pg_engine: Engine) -> str:
    name = "qi_test"
    with pg_engine.begin() as conn:
        conn.execute(text(f'CREATE SCHEMA IF NOT EXISTS "{name}"'))
    return name
