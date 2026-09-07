#!/bin/bash
# Runs the Linux metrics executable inside the Parallels VM from a macOS host.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Fedora 42 ARM64}"
linux_share="${LINUX_SHARE:-/media/psf/$(basename "$build_dir")}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run capture-our-metrics.sh directly inside Linux" >&2
  exit 1
fi

# The guest-side script owns all loops and arguments containing spaces. This avoids prlctl's
# lossy argv forwarding while leaving the actual conformance runner Linux-native.
exec prlctl exec "$vm" env \
  METRICS_LIMIT="${METRICS_LIMIT:-0}" \
  bash "$linux_share/capture-our-metrics.sh"
