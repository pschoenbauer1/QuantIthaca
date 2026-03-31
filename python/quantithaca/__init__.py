"""QuantIthaca Python API."""

from importlib import import_module
from pathlib import Path
import sys


def _load_core_bind():
    try:
        return import_module("core_bind")
    except ModuleNotFoundError:
        repo_root = Path(__file__).resolve().parents[2]
        for pyd in repo_root.glob("build/**/core_bind*.pyd"):
            sys.path.insert(0, str(pyd.parent))
            try:
                return import_module("core_bind")
            except ModuleNotFoundError:
                continue
        raise


_core_bind = _load_core_bind()
add = _core_bind.add
cpp_callback = _core_bind.cpp_callback

__all__ = ["add", "cpp_callback"]
