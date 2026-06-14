from init import init

init()

import streamlit as st

from display import apply_full_width

st.set_page_config(page_title="QuantIthaca", layout="wide")
apply_full_width()
st.title("QuantIthaca")
st.markdown("Select an app from the sidebar.")
