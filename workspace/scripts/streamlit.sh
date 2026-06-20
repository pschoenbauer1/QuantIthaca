#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

port="${1:-10000}"

log_dir="$HOME/.local/share/quantithaca"
mkdir -p "$log_dir"
log_file="$log_dir/streamlit.log"

uv run --no-project streamlit run apps/streamlit/st_main.py --server.port "$port" \
    >> "$log_file" 2>&1 &

echo $! > "$log_dir/streamlit.pid"
echo "Started (pid $!). Logs: $log_file"
