from datetime import date, datetime

import pytest

from quantithaca.utils.dateutils import to_date, to_datetime


@pytest.mark.parametrize(
    "s,expected",
    [
        ("15/03/2024", date(2024, 3, 15)),
        ("15.03.2024", date(2024, 3, 15)),
        ("2024-03-15", date(2024, 3, 15)),
        ("20240315", date(2024, 3, 15)),
    ],
)
def test_to_date_string_formats(s, expected):
    assert to_date(s) == expected


@pytest.mark.parametrize(
    "s",
    ["03/15/2024", "2024/03/15", "15-03-2024", "24-03-15", "", "not-a-date"],
)
def test_to_date_string_rejects(s):
    with pytest.raises(ValueError):
        to_date(s)


def test_to_date_from_datetime():
    assert to_date(datetime(2024, 6, 1, 14, 30)) == date(2024, 6, 1)


def test_to_date_from_date():
    d = date(2024, 6, 1)
    assert to_date(d) is d


def test_to_datetime_from_string_midnight():
    assert to_datetime("2024-03-15") == datetime(2024, 3, 15, 0, 0, 0)


def test_to_datetime_d_m_y_hms_colon_ms():
    assert to_datetime("31.12.2024 22:28:19:841") == datetime(
        2024, 12, 31, 22, 28, 19, 841_000
    )


def test_to_datetime_d_m_y_hms_colon_single_digit_ms():
    assert to_datetime("01.01.2024 00:00:00:1") == datetime(2024, 1, 1, 0, 0, 0, 1_000)


@pytest.mark.parametrize(
    "s,expected",
    [
        ("31.12.2024 22:28:19", datetime(2024, 12, 31, 22, 28, 19)),
        ("31.12.2024 22:28:19.841", datetime(2024, 12, 31, 22, 28, 19, 841_000)),
        ("2024-12-31 09:05", datetime(2024, 12, 31, 9, 5, 0)),
        ("2024-12-31 09:05:06", datetime(2024, 12, 31, 9, 5, 6)),
        ("2024-12-31 09:05:06:12", datetime(2024, 12, 31, 9, 5, 6, 12_000)),
        ("2024-12-31 09:05:06.123456", datetime(2024, 12, 31, 9, 5, 6, 123_456)),
        ("20240315 14:30", datetime(2024, 3, 15, 14, 30, 0)),
        ("14:30 15/03/2024", datetime(2024, 3, 15, 14, 30, 0)),
        ("22:28:19:841 31.12.2024", datetime(2024, 12, 31, 22, 28, 19, 841_000)),
    ],
)
def test_to_datetime_date_time_and_time_date(s, expected):
    assert to_datetime(s) == expected


def test_to_datetime_from_date():
    assert to_datetime(date(2024, 3, 15)) == datetime(2024, 3, 15, 0, 0, 0)


def test_to_datetime_from_datetime_unchanged():
    dt = datetime(2024, 3, 15, 9, 30, 5)
    assert to_datetime(dt) is dt


def test_to_date_type_error():
    with pytest.raises(TypeError):
        to_date(20240315)  # type: ignore[arg-type]
