#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

port="${1:-10000}"

uv run --no-project streamlit run apps/streamlit/st_main.py --server.port "$port"
