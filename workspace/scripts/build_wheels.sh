#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

if [[ "$#" -gt 0 ]]; then
  python_versions=("$@")
else
  python_versions=("3.11" "3.12" "3.13")
fi
mkdir -p wheelhouse

for py in "${python_versions[@]}"; do
  uv python install "$py"
  uv build --wheel --python "$py" --out-dir wheelhouse
done
