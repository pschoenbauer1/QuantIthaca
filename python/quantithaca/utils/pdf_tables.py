"""Render a mapping of pandas DataFrames into a multi-section PDF (ReportLab)."""

from __future__ import annotations

from datetime import date, datetime, time
from html import escape
from pathlib import Path
from typing import Any, Callable, Literal, Mapping, TypedDict

import numpy as np
import pandas as pd
from reportlab.lib import colors
from reportlab.lib.pagesizes import landscape as pagesize_landscape, letter
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.platypus import LongTable, PageBreak, Paragraph, SimpleDocTemplate, Spacer, TableStyle

__all__ = ["dataframes_to_pdf", "PdfColumnSpec"]

_HEADER_BG = colors.HexColor("#2F5496")
_GROUP_BG = colors.HexColor("#D6EAF6")
_TOTAL_BG = colors.HexColor("#E2E8F0")

RowTag = Literal["column_header", "group", "data", "grand_total", "total"]

AggName = Literal["sum", "mean", "min", "max", "count", "median", "std", "first", "last"]
AggCallable = Callable[[pd.Series], Any]
AggOption = AggName | AggCallable | None


class PdfColumnSpec(TypedDict, total=False):
    """
    Per-column PDF behaviour for aggregate cells (group headers, totals).

    * ``agg``: how to aggregate rows in a branch; ``None`` leaves the cell empty.
      Use a name (e.g. ``\"sum\"``) or ``Callable[[Series], Any]``.
    * ``float_format``: format for numeric display, e.g. ``\"{:.2f}\"`` or ``\".2f\"``
      (passed to ``format(value, spec)`` when no ``{`` is present).
    """

    agg: AggOption
    float_format: str | None


def _default_agg_for_series(s: pd.Series) -> AggOption:
    if pd.api.types.is_numeric_dtype(s):
        return "sum"
    return None


def _effective_spec(col: str, series: pd.Series, specs: Mapping[str, PdfColumnSpec]) -> PdfColumnSpec:
    raw = specs.get(col, {})
    if "agg" in raw:
        agg: AggOption = raw["agg"]
    else:
        agg = _default_agg_for_series(series)
    fmt = raw.get("float_format")
    return {"agg": agg, "float_format": fmt}


def _is_midnight_timestamp(ts: pd.Timestamp) -> bool:
    return (
        ts.hour == 0
        and ts.minute == 0
        and ts.second == 0
        and ts.microsecond == 0
        and getattr(ts, "nanosecond", 0) == 0
    )


def _display_scalar(val: Any) -> str:
    """String for PDF cells; date-only datetimes show as YYYY-MM-DD (no 00:00:00)."""
    if val is None:
        return ""
    if isinstance(val, float) and pd.isna(val):
        return ""
    if pd.isna(val):
        return ""
    if isinstance(val, pd.Timestamp):
        if _is_midnight_timestamp(ts=val):
            return val.date().isoformat()
        return str(val)
    if isinstance(val, datetime):
        if val.time() == time.min:
            return val.date().isoformat()
        return str(val)
    if isinstance(val, date):
        return val.isoformat()
    if isinstance(val, np.datetime64):
        tsn = pd.Timestamp(val)
        if _is_midnight_timestamp(tsn):
            return tsn.date().isoformat()
        return str(tsn)
    return str(val)


def _run_agg(series: pd.Series, agg: AggOption) -> Any:
    if agg is None:
        return None
    if callable(agg):
        return agg(series)
    if agg == "sum":
        return series.sum()
    if agg == "mean":
        return series.mean()
    if agg == "min":
        return series.min()
    if agg == "max":
        return series.max()
    if agg == "count":
        return series.count()
    if agg == "median":
        return series.median()
    if agg == "std":
        return series.std()
    if agg == "first":
        return series.iloc[0] if len(series) else None
    if agg == "last":
        return series.iloc[-1] if len(series) else None
    raise ValueError(f"Unknown aggregation: {agg!r}")


def _format_cell_value(val: Any, float_format: str | None) -> str:
    if val is None:
        return ""
    if isinstance(val, float) and pd.isna(val):
        return ""
    if pd.isna(val):
        return ""
    if float_format:
        try:
            if isinstance(val, (int, float)) and not isinstance(val, bool):
                if "{" in float_format:
                    return float_format.format(val)
                return format(val, float_format)
        except (ValueError, TypeError):
            pass
    return _display_scalar(val)


def _aggregate_cell(
    col: str,
    sf: pd.DataFrame,
    spec: PdfColumnSpec,
) -> str:
    if col not in sf.columns:
        return ""
    series = sf[col]
    agg = spec.get("agg")
    if agg is None:
        return ""
    val = _run_agg(series, agg)
    return _format_cell_value(val, spec.get("float_format"))


def _group_header_with_aggs(
    depth: int,
    key: Any,
    headers: list[str],
    sf: pd.DataFrame,
    specs: Mapping[str, PdfColumnSpec],
    n_index_cols: int,
) -> list[str]:
    row = [""] * len(headers)
    row[depth] = _display_scalar(key)
    for j, h in enumerate(headers):
        if j < n_index_cols:
            continue
        spec = _effective_spec(h, sf[h], specs)
        row[j] = _aggregate_cell(h, sf, spec)
    return row


def _total_like_row(
    headers: list[str],
    sf: pd.DataFrame,
    specs: Mapping[str, PdfColumnSpec],
    n_index_cols: int,
    label: str,
) -> list[str]:
    row = [""] * len(headers)
    row[0] = label
    for j, h in enumerate(headers):
        if j == 0:
            continue
        if j < n_index_cols:
            row[j] = ""
            continue
        spec = _effective_spec(h, sf[h], specs)
        row[j] = _aggregate_cell(h, sf, spec)
    return row


def _format_data_value(col: str, val: Any, specs: Mapping[str, PdfColumnSpec], series_hint: pd.Series) -> str:
    if pd.isna(val):
        return ""
    spec = _effective_spec(col, series_hint, specs)
    fmt = spec.get("float_format")
    if fmt and isinstance(val, (int, float)) and not isinstance(val, bool):
        return _format_cell_value(val, fmt)
    return _display_scalar(val)


def _dataframe_to_grid(
    flat: pd.DataFrame,
    *,
    append_total: bool,
    specs: Mapping[str, PdfColumnSpec],
    n_index_cols: int,
) -> list[list[str]]:
    headers = [str(c) for c in flat.columns]
    if flat.empty:
        grid = [headers]
        if append_total and headers:
            grid.append(_total_like_row(headers, flat, specs, n_index_cols, "Total"))
        return grid
    body: list[list[str]] = []
    for _, r in flat.iterrows():
        row = []
        for c in flat.columns:
            row.append(_format_data_value(c, r[c], specs, flat[c]))
        body.append(row)
    grid = [headers] + body
    if append_total:
        grid.append(_total_like_row(headers, flat, specs, n_index_cols, "Total"))
    return grid


def _build_aggrid_style_grid(
    df: pd.DataFrame,
    specs: Mapping[str, PdfColumnSpec],
) -> tuple[list[list[str]], list[RowTag]]:
    """Single table: group header rows include branch aggregates; grand total at bottom."""
    full_flat = df.reset_index()
    headers = [str(c) for c in full_flat.columns]
    n_index_cols = df.index.nlevels
    ncols = len(headers)
    grid: list[list[str]] = [headers]
    tags: list[RowTag] = ["column_header"]

    def walk(slice_df: pd.DataFrame, path: tuple) -> None:
        if slice_df.index.nlevels == 1:
            part = slice_df.reset_index()
            for _, r in part.iterrows():
                cells = [_display_scalar(p) for p in path]
                for c in part.columns:
                    cells.append(_format_data_value(c, r[c], specs, part[c]))
                grid.append(cells)
                tags.append("data")
            return

        for key, sub in slice_df.groupby(level=0, sort=True):
            sf = sub.reset_index()
            grid.append(_group_header_with_aggs(len(path), key, headers, sf, specs, n_index_cols))
            tags.append("group")
            dropped = sub.reset_index(level=0, drop=True)
            walk(dropped, path + (key,))

    walk(df, ())
    grid.append(_total_like_row(headers, full_flat, specs, n_index_cols, "Grand total"))
    tags.append("grand_total")
    return grid, tags


def _table_style_aggrid(grid: list[list[str]], tags: list[RowTag]) -> TableStyle:
    base: list[tuple] = [
        ("BACKGROUND", (0, 0), (-1, 0), _HEADER_BG),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.whitesmoke),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
        ("FONTSIZE", (0, 0), (-1, 0), 9),
        ("BOTTOMPADDING", (0, 0), (-1, 0), 8),
        ("TOPPADDING", (0, 0), (-1, 0), 6),
        ("ALIGN", (0, 0), (-1, -1), "LEFT"),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("GRID", (0, 0), (-1, -1), 0.25, colors.grey),
    ]
    data_stripe = 0
    for ri in range(1, len(grid)):
        tag = tags[ri]
        if tag == "group":
            data_stripe = 0
            base.extend(
                [
                    ("FONTNAME", (0, ri), (-1, ri), "Helvetica-Bold"),
                    ("BACKGROUND", (0, ri), (-1, ri), _GROUP_BG),
                    ("FONTSIZE", (0, ri), (-1, ri), 9),
                ]
            )
        elif tag == "data":
            bg = colors.white if data_stripe % 2 == 0 else colors.HexColor("#F5F5F5")
            base.extend(
                [
                    ("FONTNAME", (0, ri), (-1, ri), "Helvetica"),
                    ("FONTSIZE", (0, ri), (-1, ri), 8),
                    ("BACKGROUND", (0, ri), (-1, ri), bg),
                ]
            )
            data_stripe += 1
        elif tag in ("grand_total", "total"):
            base.extend(
                [
                    ("FONTNAME", (0, ri), (-1, ri), "Helvetica-Bold"),
                    ("FONTSIZE", (0, ri), (-1, ri), 9),
                    ("BACKGROUND", (0, ri), (-1, ri), _TOTAL_BG),
                ]
            )
    return TableStyle(base)


def _long_table_from_grid(
    grid: list[list[str]],
    doc: SimpleDocTemplate,
    *,
    style: TableStyle,
) -> LongTable:
    ncols = max(len(grid[0]), 1)
    col_w = doc.width / ncols
    col_widths = [col_w] * ncols
    t = LongTable(grid, colWidths=col_widths, repeatRows=1)
    t.setStyle(style)
    return t


def _append_dataframe_as_grouped_tables(
    story: list,
    df: pd.DataFrame,
    doc: SimpleDocTemplate,
    styles,
    specs: Mapping[str, PdfColumnSpec],
) -> None:
    if df.empty:
        story.append(Paragraph(escape("(empty table)"), styles["Normal"]))
        return

    n_index = df.index.nlevels if isinstance(df.index, pd.MultiIndex) else 1

    if isinstance(df.index, pd.MultiIndex) and df.index.nlevels > 1:
        grid, tags = _build_aggrid_style_grid(df, specs)
        style = _table_style_aggrid(grid, tags)
        story.append(_long_table_from_grid(grid, doc, style=style))
        return

    flat = df.reset_index()
    grid = _dataframe_to_grid(flat, append_total=True, specs=specs, n_index_cols=n_index)
    n = len(grid)
    if n <= 1:
        tags: list[RowTag] = ["column_header"] if n == 1 else []
    elif n == 2:
        tags = ["column_header", "total"]
    else:
        tags = ["column_header"] + ["data"] * (n - 2) + ["total"]
    style = _table_style_aggrid(grid, tags)
    story.append(_long_table_from_grid(grid, doc, style=style))


def dataframes_to_pdf(
    data: Mapping[str, pd.DataFrame],
    path: str | Path,
    *,
    pagesize: tuple[float, float] = letter,
    landscape: bool = False,
    section_column_specs: Mapping[str, Mapping[str, PdfColumnSpec]] | None = None,
    overwrite: bool = False,
) -> None:
    """
    Write a PDF where each key is a section title and each value is one table.

    * **MultiIndex (2+ levels)**: one table — each **group row** shows the level label in its
      depth column and **aggregates in value columns** (no separate Subtotal row). **Grand total**
      at the bottom.
    * **Flat index**: one table with a **Total** row.

    ``section_column_specs`` maps the same keys as ``data`` to per-column :class:`PdfColumnSpec`
    (optional). Unlisted columns default to ``sum`` for numeric data and no aggregate otherwise.

    If ``overwrite`` is ``False`` (default), raises :exc:`FileExistsError` when ``path``
    already exists. Set ``overwrite=True`` to replace an existing file.
    """
    if not data:
        raise ValueError("data must contain at least one DataFrame.")

    path = Path(path)
    if path.exists() and not overwrite:
        raise FileExistsError(f"Output path already exists (refusing to overwrite): {path}")
    path.parent.mkdir(parents=True, exist_ok=True)

    effective_pagesize = pagesize_landscape(pagesize) if landscape else pagesize

    doc = SimpleDocTemplate(
        str(path),
        pagesize=effective_pagesize,
        leftMargin=72,
        rightMargin=72,
        topMargin=72,
        bottomMargin=72,
    )
    styles = getSampleStyleSheet()
    title_style = styles["Heading2"]

    specs_by_section: Mapping[str, Mapping[str, PdfColumnSpec]] = section_column_specs or {}

    story: list = []
    items = list(data.items())
    for i, (title, df) in enumerate(items):
        if not isinstance(df, pd.DataFrame):
            raise TypeError(f"Value for key {title!r} must be a pandas DataFrame.")
        story.append(Paragraph(escape(str(title)), title_style))
        story.append(Spacer(1, 12))
        sec_specs = dict(specs_by_section.get(title, {}))
        _append_dataframe_as_grouped_tables(story, df, doc, styles, sec_specs)
        story.append(Spacer(1, 18))
        if i < len(items) - 1:
            story.append(PageBreak())

    doc.build(story)
