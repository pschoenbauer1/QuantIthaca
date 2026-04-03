"""Central logging for QuantIthaca (stdlib ``logging`` wrapper).

Use::

    from quantithaca.logging import logger

    logger.info("INFO")
    logger.warning("WARNING")
"""

from __future__ import annotations

import logging
import sys
from datetime import datetime

__all__ = ["logger"]


class _QuantFormatter(logging.Formatter):
    """``datetime | filename | lineno | pid: message`` (with milliseconds)."""

    def formatTime(self, record: logging.LogRecord, datefmt: str | None = None) -> str:
        dt = datetime.fromtimestamp(record.created)
        ms = int(record.msecs)
        return dt.strftime("%Y-%m-%d %H:%M:%S") + f".{ms:03d}"


def _configure_logger() -> logging.Logger:
    log = logging.getLogger("quantithaca")
    if log.handlers:
        return log

    log.setLevel(logging.INFO)
    handler = logging.StreamHandler(sys.stderr)
    fmt = "%(asctime)s | %(filename)s:%(lineno)d | %(process)d: %(message)s"
    handler.setFormatter(_QuantFormatter(fmt))
    log.addHandler(handler)
    log.propagate = False
    return log


logger = _configure_logger()
