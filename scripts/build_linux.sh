#!/usr/bin/env bash
set -euo pipefail

compiler="${1:-gcc}"   # gcc|clang
config="${2:-Debug}"   # Debug|Release
python_version="${3:-3.12}"
run_tests="${4:-false}" # true|false

case "$compiler" in
  gcc)
    if [[ "$config" == "Debug" ]]; then preset="linux-gcc-debug"; else preset="linux-gcc-release"; fi
    ;;
  clang)
    if [[ "$config" == "Debug" ]]; then preset="linux-clang-debug"; else preset="linux-clang-release"; fi
    ;;
  *)
    echo "Unsupported compiler: $compiler"
    exit 1
    ;;
esac

uv sync --python "$python_version" --extra test
uv run --python "$python_version" cmake --preset "$preset"
uv run --python "$python_version" cmake --build --preset "$preset"

if [[ "$run_tests" == "true" ]]; then
  uv run --python "$python_version" ctest --preset "$preset" --output-on-failure
  uv run --python "$python_version" pytest
fi
