#!/usr/bin/env bash
set -euo pipefail

mkdir -p wheelhouse
for py in 3.11 3.12 3.13; do
  tag="quantithaca-wheel:${py}"
  docker build \
    --build-arg PYTHON_VERSION="$py" \
    -f docker/Dockerfile \
    -t "$tag" \
    .

  container_id="$(docker create "$tag")"
  docker cp "${container_id}:/workspace/wheelhouse/." wheelhouse/
  docker rm "$container_id" >/dev/null
done
