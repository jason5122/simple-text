#!/bin/bash
# Captures the native Windows buffer conformance suite through Parallels.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Windows 11}"
windows_share="${WINDOWS_SHARE:-\\\\Mac\\win-arm64}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run this script from the macOS host" >&2
  exit 1
fi

rm -rf "$build_dir/ours"
prlctl exec "$vm" --current-user \
  "$windows_share\\buffer_conformance.exe" \
  "$windows_share\\capture-tests" \
  "$windows_share\\ours" \
  "$@"
