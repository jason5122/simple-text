#!/bin/bash
# Starts the Linux-native sidebar capture suite in the Parallels VM from a macOS host.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Fedora 42 ARM64}"
linux_share="${LINUX_SHARE:-/media/psf/$(basename "$build_dir")}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run capture-sublime-ui.sh directly inside Linux" >&2
  exit 1
fi

exec prlctl exec "$vm" env \
  LINUX_CAPTURE_BACKEND="${LINUX_CAPTURE_BACKEND:-auto}" \
  LINUX_CAPTURE_LOGICAL_TOP_INSET="${LINUX_CAPTURE_LOGICAL_TOP_INSET:-37}" \
  LINUX_UI_CAPTURE_TESTS="${LINUX_UI_CAPTURE_TESTS:-$linux_share/capture-tests-ui}" \
  LINUX_UI_SUBLIME_OUTPUT="${LINUX_UI_SUBLIME_OUTPUT:-$linux_share/sublime-ui}" \
  UI_CROP="${UI_CROP:-0,0,600,500}" \
  bash "$linux_share/capture-sublime-ui.sh"
