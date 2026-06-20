import pandas as pd
import pytest

from quantithaca.utils.pdf_tables import PdfColumnSpec, _display_scalar, dataframes_to_pdf


def test_dataframes_to_pdf_creates_file(tmp_path):
    out = tmp_path / "out.pdf"
    data = {
        "First": pd.DataFrame({"a": [1, 2], "b": ["x", "y"]}),
        "Second": pd.DataFrame({"n": [10]}),
    }
    dataframes_to_pdf(data, out)
    assert out.is_file()
    assert out.stat().st_size > 500


def test_dataframes_to_pdf_no_trailing_blank_page(tmp_path):
    """A trailing Spacer after the last table used to orphan an empty final page."""
    pytest.importorskip("pypdf")
    from pypdf import PdfReader

    out = tmp_path / "one.pdf"
    df = pd.DataFrame({"a": range(30), "b": ["x"] * 30})
    dataframes_to_pdf({"S": df}, out)
    assert len(PdfReader(str(out)).pages) == 1


def test_dataframes_to_pdf_multi_section_page_break(tmp_path):
    pytest.importorskip("pypdf")
    from pypdf import PdfReader

    out = tmp_path / "two.pdf"
    dataframes_to_pdf(
        {"A": pd.DataFrame({"a": [1]}), "B": pd.DataFrame({"b": [2]})},
        out,
        show_header_date=False,
        show_page_numbers=False,
    )
    assert len(PdfReader(str(out)).pages) == 2


def test_dataframes_to_pdf_landscape(tmp_path):
    out = tmp_path / "landscape.pdf"
    data = {"Wide": pd.DataFrame({f"c{i}": [i] for i in range(12)})}
    dataframes_to_pdf(data, out, landscape=True)
    assert out.is_file()
    assert out.stat().st_size > 500


def test_dataframes_to_pdf_empty_dict_raises():
    with pytest.raises(ValueError, match="at least one"):
        dataframes_to_pdf({}, "nope.pdf")


def test_dataframes_to_pdf_raises_if_file_exists(tmp_path):
    out = tmp_path / "taken.pdf"
    out.write_bytes(b"%PDF-1.4")
    with pytest.raises(FileExistsError, match="already exists"):
        dataframes_to_pdf(
            {"S": pd.DataFrame({"a": [1]})},
            out,
        )


def test_dataframes_to_pdf_overwrite_true_replaces_file(tmp_path):
    out = tmp_path / "replace.pdf"
    out.write_bytes(b"%PDF-1.4")
    stub_size = out.stat().st_size
    dataframes_to_pdf({"S": pd.DataFrame({"a": [1, 2, 3]})}, out, overwrite=True)
    assert out.stat().st_size > stub_size


def test_dataframes_to_pdf_rejects_non_dataframe(tmp_path):
    with pytest.raises(TypeError, match="DataFrame"):
        dataframes_to_pdf({"x": "not a df"}, tmp_path / "x.pdf")


def test_dataframes_to_pdf_section_column_specs(tmp_path):
    out = tmp_path / "specs.pdf"
    df = pd.DataFrame({"x": [10, 20], "y": [1.111, 2.222]})
    specs: dict[str, PdfColumnSpec] = {
        "x": {"agg": None},
        "y": {"agg": "mean", "float_format": "{:.2f}"},
    }
    dataframes_to_pdf({"T": df}, out, section_column_specs={"T": specs})
    assert out.is_file()
    assert out.stat().st_size > 400


def test_display_scalar_date_without_time():
    assert _display_scalar(pd.Timestamp("2024-03-15 00:00:00")) == "2024-03-15"
    assert "00:00:00" not in _display_scalar(pd.Timestamp("2024-03-15 00:00:00"))
    assert _display_scalar(pd.Timestamp("2024-03-15 14:30:00")) == "2024-03-15 14:30:00"


def test_dataframes_to_pdf_header_footer_and_page_numbers(tmp_path):
    out = tmp_path / "hf.pdf"
    dataframes_to_pdf(
        {"S": pd.DataFrame({"a": [1]})},
        out,
        header_text="Quarterly report",
        footer_text="Confidential",
    )
    assert out.is_file()
    assert out.stat().st_size > 400


def test_dataframes_to_pdf_custom_page_number_format(tmp_path):
    out = tmp_path / "pn.pdf"
    dataframes_to_pdf(
        {"S": pd.DataFrame({"a": [1]})},
        out,
        page_number_format="- {page} -",
    )
    assert out.is_file()


def test_dataframes_to_pdf_header_date_only(tmp_path):
    out = tmp_path / "date_only.pdf"
    dataframes_to_pdf(
        {"S": pd.DataFrame({"a": [1]})},
        out,
        show_page_numbers=False,
        header_date_format="%Y/%m/%d",
    )
    assert out.is_file()


def test_dataframes_to_pdf_no_page_decorations(tmp_path):
    out = tmp_path / "plain.pdf"
    dataframes_to_pdf(
        {"S": pd.DataFrame({"a": [1]})},
        out,
        show_page_numbers=False,
        show_header_date=False,
    )
    assert out.is_file()


def test_dataframes_to_pdf_page_number_format_requires_placeholder(tmp_path):
    with pytest.raises(ValueError, match="page_number_format"):
        dataframes_to_pdf(
            {"S": pd.DataFrame({"a": [1]})},
            tmp_path / "bad.pdf",
            page_number_format="nope",
        )


def test_dataframes_to_pdf_multiindex_grouped(tmp_path):
    out = tmp_path / "multi.pdf"
    idx = pd.MultiIndex.from_tuples(
        [("a", "x"), ("a", "y"), ("b", "x")],
        names=["outer", "inner"],
    )
    df = pd.DataFrame({"qty": [1, 2, 3], "amt": [10.0, 20.0, 30.0]}, index=idx)
    dataframes_to_pdf({"Grouped": df}, out)
    assert out.is_file()
    assert out.stat().st_size > 800
