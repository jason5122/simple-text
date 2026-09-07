#!/bin/bash
# Runs Sublime's metrics probe inside the Parallels Linux VM from a macOS host.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Fedora 42 ARM64}"
linux_share="${LINUX_SHARE:-/media/psf/$(basename "$build_dir")}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run capture-sublime-metrics.sh directly inside Linux" >&2
  exit 1
fi

exec prlctl exec "$vm" env \
  METRICS_LIMIT="${METRICS_LIMIT:-0}" \
  bash "$linux_share/capture-sublime-metrics.sh"
