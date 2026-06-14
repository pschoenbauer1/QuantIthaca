"""Shared startup for Streamlit apps."""

import sys
from pathlib import Path


def init() -> bool:
    """Add `python/` to sys.path and load `.env` from the repo root."""
    repo_root = Path(__file__).resolve().parents[2]
    python_root = repo_root / "python"
    python_root_str = str(python_root)
    if python_root_str not in sys.path:
        sys.path.insert(0, python_root_str)

    try:
        from dotenv import find_dotenv, load_dotenv
    except ImportError:
        return False

    dotenv_path = repo_root / ".env"
    if dotenv_path.exists():
        return load_dotenv(dotenv_path)

    path = find_dotenv(usecwd=True)
    if not path:
        return False
    return load_dotenv(path)
