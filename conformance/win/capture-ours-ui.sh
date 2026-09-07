#!/bin/bash
# Captures native Windows UI conformance cases through Parallels.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Windows 11}"
windows_share="${WINDOWS_SHARE:-\\\\Mac\\$(basename "$build_dir")}"
crop="${1:-0,0,600,500}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run ui_conformance.exe directly inside Windows" >&2
  exit 1
fi

rm -rf "$build_dir/ours-ui"
prlctl exec "$vm" --current-user \
  "$windows_share\\ui_conformance.exe" \
  "$windows_share\\capture-tests-ui" \
  "$windows_share\\ours-ui" \
  --crop "$crop"
