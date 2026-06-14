from init import init

init()

import streamlit as st

from display import apply_full_width, editable_dataframe, show_dataframe
from quantithaca.finance.rlb_cashflows import (
    cashflows_to_pnl,
    enrich_cashflows,
    read_rlb_cashflows_sql,
    save_rlb_cashflows_is_private,
    write_pnl,
)

st.set_page_config(page_title="RLB Cashflows", layout="wide")
apply_full_width()
st.title("RLB Cashflows")

tab_raw, tab_enriched = st.tabs(["Raw", "Enriched"])

with tab_raw:
    if st.button("Load raw cashflows", key="load_raw"):
        with st.spinner("Loading from database..."):
            st.session_state["raw_cashflows"] = read_rlb_cashflows_sql()

    if "raw_cashflows" in st.session_state:
        raw = st.session_state["raw_cashflows"]
        st.caption(f"{len(raw)} rows")
        show_dataframe(raw, height=1000)

with tab_enriched:
    if st.button("Load enriched cashflows", key="load_enriched"):
        with st.spinner("Loading and applying allocation rules..."):
            st.session_state["enriched_cashflows"] = enrich_cashflows()
            st.session_state.pop("enrich_error", None)

    if "enriched_cashflows" in st.session_state:
        enriched = st.session_state["enriched_cashflows"]
        error_count = enriched["error"].notna().sum()
        st.caption(f"{len(enriched)} rows ({error_count} with errors)")

        edited = editable_dataframe(
            enriched,
            editable_cols=["is_private"],
            checkbox_cols=["is_private"],
            height=1000,
            key="enriched_editor",
        )

        has_unsaved_edits = (
            enriched.reset_index(drop=True)["is_private"].astype(bool)
            != edited.reset_index(drop=True)["is_private"].astype(bool)
        ).any()

        if st.button(
            "Write P&L (incremental)",
            key="write_pnl_incremental",
            disabled=has_unsaved_edits,
        ):
            try:
                allocated = cashflows_to_pnl(enriched)
                inserted = write_pnl(allocated, mode="insert_incremental")
                st.session_state["write_pnl_message"] = f"Inserted {inserted} new P&L row(s)."
                st.session_state.pop("write_pnl_error", None)
            except ValueError as exc:
                st.session_state["write_pnl_error"] = str(exc)
                st.session_state.pop("write_pnl_message", None)

        if st.button("Save is_private", key="save_is_private"):
            try:
                updated = save_rlb_cashflows_is_private(enriched, edited)
                st.session_state["enriched_cashflows"] = enrich_cashflows()
                st.session_state["save_is_private_message"] = f"Updated {updated} row(s)."
                st.session_state.pop("save_is_private_error", None)
            except ValueError as exc:
                st.session_state["save_is_private_error"] = str(exc)
                st.session_state.pop("save_is_private_message", None)

        if has_unsaved_edits:
            st.caption("Save or revert is_private changes before writing P&L.")

        if st.session_state.get("save_is_private_error"):
            st.error(st.session_state["save_is_private_error"])
        if st.session_state.get("save_is_private_message"):
            st.success(st.session_state["save_is_private_message"])
        if st.session_state.get("write_pnl_error"):
            st.error(st.session_state["write_pnl_error"])
        if st.session_state.get("write_pnl_message"):
            st.success(st.session_state["write_pnl_message"])
