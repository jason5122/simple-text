#!/bin/bash
# Runs one native Linux window capture in the Parallels VM from a macOS host.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Fedora 42 ARM64}"
linux_share="${LINUX_SHARE:-/media/psf/$(basename "$build_dir")}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run capture-once.sh directly inside Linux" >&2
  exit 1
fi

exec prlctl exec "$vm" env \
  LINUX_CAPTURE_BACKEND="${LINUX_CAPTURE_BACKEND:-auto}" \
  LINUX_CAPTURE_CROP="${LINUX_CAPTURE_CROP:-0,0,0,0}" \
  LINUX_CAPTURE_OUTPUT="${LINUX_CAPTURE_OUTPUT:-$linux_share/capture.png}" \
  LINUX_CAPTURE_OWNER="${LINUX_CAPTURE_OWNER:-capture-target}" \
  LINUX_CAPTURE_PID="${LINUX_CAPTURE_PID:-0}" \
  bash "$linux_share/capture-once.sh"
