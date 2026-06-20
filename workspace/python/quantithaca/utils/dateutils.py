"""Parse dates from a small set of string formats into Python ``date`` / ``datetime``."""

from __future__ import annotations

import re
from datetime import date, datetime, time
from typing import overload

__all__ = ["to_date", "to_datetime"]

# String formats accepted for *date-only* input (strict; no other layouts).
_DATE_STRPTIME: tuple[tuple[str, str], ...] = (
    ("%d/%m/%Y", "d/m/Y"),
    ("%d.%m.%Y", "d.m.Y"),
    ("%Y-%m-%d", "Y-m-d"),
    ("%Y%m%d", "Ymd"),
)

# Time: try longer / more specific patterns first (fractional before plain HMS before HM).
_TIME_WITH_FRAC = re.compile(
    r"^(\d{1,2}):(\d{1,2}):(\d{1,2})([.:])(\d{1,6})$"
)
_TIME_HMS = re.compile(r"^(\d{1,2}):(\d{1,2}):(\d{1,2})$")
_TIME_HM = re.compile(r"^(\d{1,2}):(\d{1,2})$")


def _frac_to_microseconds(frac_s: str) -> int:
    n = len(frac_s)
    if not 1 <= n <= 6:
        raise ValueError("Fractional seconds must be 1–6 digits.")
    if n <= 3:
        micro = int(frac_s) * 1000
    else:
        micro = int(frac_s)
    if micro >= 1_000_000:
        raise ValueError("Sub-second value out of range.")
    return micro


def _parse_time_part(s: str) -> tuple[int, int, int, int]:
    raw = s.strip()
    m = _TIME_WITH_FRAC.fullmatch(raw)
    if m:
        hh_s, mm_s, ss_s, _sep, frac_s = m.groups()
        h, mi, sec = int(hh_s), int(mm_s), int(ss_s)
        micro = _frac_to_microseconds(frac_s)
    else:
        m = _TIME_HMS.fullmatch(raw)
        if m:
            h, mi, sec = int(m.group(1)), int(m.group(2)), int(m.group(3))
            micro = 0
        else:
            m = _TIME_HM.fullmatch(raw)
            if not m:
                raise ValueError(f"Time string {raw!r} is not valid.")
            h, mi = int(m.group(1)), int(m.group(2))
            sec, micro = 0, 0
    if not (0 <= h <= 23 and 0 <= mi <= 59 and 0 <= sec <= 59):
        raise ValueError(f"Time out of range in {raw!r}.")
    return h, mi, sec, micro


def _parse_date_string(s: str) -> date:
    raw = s.strip()
    if not raw:
        raise ValueError("Empty date string is not allowed.")
    for fmt, _label in _DATE_STRPTIME:
        try:
            return datetime.strptime(raw, fmt).date()
        except ValueError:
            continue
    allowed = ", ".join(label for _fmt, label in _DATE_STRPTIME)
    raise ValueError(
        f"Date string {raw!r} does not match any allowed format ({allowed})."
    )


def _try_parse_datetime_date_and_time(raw: str) -> datetime | None:
    """
    Parse ``DATE TIME`` or ``TIME DATE`` (single whitespace run between tokens).

    Tokens are split on whitespace; every non-empty cut between token runs is tried
    so ``a b c`` can pair ``(a)(b c)`` or ``(a b)(c)``.
    """
    parts = raw.split()
    if len(parts) < 2:
        return None
    for k in range(1, len(parts)):
        left = " ".join(parts[:k])
        right = " ".join(parts[k:])
        for date_s, time_s in ((left, right), (right, left)):
            try:
                d = _parse_date_string(date_s)
                h, mi, sec, micro = _parse_time_part(time_s)
            except ValueError:
                continue
            try:
                return datetime(d.year, d.month, d.day, h, mi, sec, micro)
            except ValueError:
                continue
    return None


def _parse_datetime_string(s: str) -> datetime:
    """Date-only → midnight; else ``DATE TIME`` / ``TIME DATE`` with allowed time shapes."""
    raw = s.strip()
    if not raw:
        raise ValueError("Empty date string is not allowed.")
    try:
        d = _parse_date_string(raw)
    except ValueError:
        combo = _try_parse_datetime_date_and_time(raw)
        if combo is not None:
            return combo
        allowed_date = ", ".join(label for _fmt, label in _DATE_STRPTIME)
        raise ValueError(
            f"Datetime string {raw!r} is not a lone date ({allowed_date}) nor "
            "DATE+TIME / TIME+DATE with HH:MM, HH:MM:SS, or HH:MM:SS plus "
            "``:milli`` / ``.milli`` (1–3 digits) or ``:micro`` / ``.micro`` (4–6 digits)."
        ) from None
    return datetime.combine(d, time.min)


@overload
def to_date(value: str) -> date: ...


@overload
def to_date(value: datetime) -> date: ...


@overload
def to_date(value: date) -> date: ...


def to_date(value: str | date | datetime) -> date:
    """
    Return a :class:`datetime.date`.

    * ``str``: must be ``d/m/Y``, ``d.m.Y``, ``Y-m-d``, or ``Ymd`` (e.g. ``20240403``).
    * :class:`datetime.datetime`: date component only.
    * :class:`datetime.date`: returned unchanged.
    """
    if isinstance(value, date) and not isinstance(value, datetime):
        return value
    if isinstance(value, datetime):
        return value.date()
    if isinstance(value, str):
        return _parse_date_string(value)
    raise TypeError(f"to_date expected str, date, or datetime; got {type(value).__name__}")


@overload
def to_datetime(value: str) -> datetime: ...


@overload
def to_datetime(value: date) -> datetime: ...


@overload
def to_datetime(value: datetime) -> datetime: ...


def to_datetime(value: str | date | datetime) -> datetime:
    """
    Return a :class:`datetime.datetime` (naive).

    * ``str``: date-only (same formats as :func:`to_date`) → ``00:00:00``, or
      ``DATE TIME`` / ``TIME DATE`` where DATE uses those formats and TIME is one of:

      * ``HH:MM``
      * ``HH:MM:SS``
      * ``HH:MM:SS:milli`` / ``HH:MM:SS.milli`` — 1–3 digits (milliseconds)
      * ``HH:MM:SS:micro`` / ``HH:MM:SS.micro`` — 4–6 digits (microseconds)

    * :class:`datetime.date`: naive midnight on that day.
    * :class:`datetime.datetime`: returned unchanged.
    """
    if isinstance(value, datetime):
        return value
    if isinstance(value, date):
        return datetime.combine(value, time.min)
    if isinstance(value, str):
        return _parse_datetime_string(value)
    raise TypeError(f"to_datetime expected str, date, or datetime; got {type(value).__name__}")
