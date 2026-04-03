"""Load Postgres credentials from the environment and build a SQLAlchemy engine (e.g. Neon)."""

from __future__ import annotations

import os
from typing import Any
from urllib.parse import quote_plus

from sqlalchemy import create_engine
from sqlalchemy.engine import Engine


def load_dotenv_optional() -> bool:
    """Load a `.env` file from the cwd or a parent directory, if present."""
    try:
        from dotenv import find_dotenv, load_dotenv
    except ImportError:
        return False
    path = find_dotenv(usecwd=True)
    if not path:
        return False
    return bool(load_dotenv(path))

def _neon_database_url(user:str) -> str | None:
    host = os.environ.get("PGHOST")
    # user = os.environ.get(f"PGUSER_{username}")
    password = os.environ.get(f"PGPASSWORD_{user.upper()}")
    database = os.environ.get("PGDATABASE")
    port = os.environ.get("PGPORT", "5432")

    if not (host and user and password and database):
        return None

    return f"postgresql+psycopg://{user}:{password}@{host}:{port}/{database}?sslmode=require"


def get_neon_env(user, *, use_dotenv: bool = True,  **engine_kwargs: Any) -> str:
    if use_dotenv:
        load_dotenv_optional()

    url = _neon_database_url(user)
    if not url:
        return None

    opts: dict[str, Any] = {"pool_pre_ping": True}
    opts.update(engine_kwargs)
    return create_engine(url, **opts)

neon_ro_engine = get_neon_env("app_ro")
neon_rw_engine = get_neon_env("app_rw")