import pandas as pd
import streamlit as st


def _max_text_len(series: pd.Series) -> int:
    if series.empty:
        return 0
    lengths = series.fillna("").astype(str).str.len()
    max_len = lengths.max()
    return 0 if pd.isna(max_len) else int(max_len)


def apply_full_width() -> None:
    st.markdown(
        """
        <style>
        .stMainBlockContainer, .block-container {
            max-width: 100%;
            padding-left: 1rem;
            padding-right: 1rem;
        }
        </style>
        """,
        unsafe_allow_html=True,
    )


def column_config_for_df(df: pd.DataFrame, *, min_width: int = 60, max_width: int = 2000) -> dict:
    config = {}
    for col in df.columns:
        header_len = len(str(col))
        content_len = _max_text_len(df[col])
        width = min(max(max(header_len, content_len) * 9 + 24, min_width), max_width)
        config[col] = st.column_config.Column(width=width)
    return config


def show_dataframe(df: pd.DataFrame, **kwargs) -> None:
    st.dataframe(
        df,
        column_config=column_config_for_df(df),
        width="stretch",
        **kwargs,
    )


def editable_dataframe(
    df: pd.DataFrame,
    editable_cols: list[str],
    *,
    checkbox_cols: list[str] | None = None,
    **kwargs,
) -> pd.DataFrame:
    checkbox_cols = checkbox_cols or []
    config = column_config_for_df(df)
    for col in checkbox_cols:
        config[col] = st.column_config.CheckboxColumn(col)

    disabled = [col for col in df.columns if col not in editable_cols]
    return st.data_editor(
        df,
        column_config=config,
        disabled=disabled,
        width="stretch",
        **kwargs,
    )
