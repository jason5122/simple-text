#!/bin/bash
# Starts the Linux-native capture suite in the Parallels VM from a macOS host.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Fedora 42 ARM64}"
linux_share="${LINUX_SHARE:-/media/psf/$(basename "$build_dir")}"
capture_tests="${LINUX_CAPTURE_TESTS:-$linux_share/capture-tests}"
capture_output="${LINUX_CAPTURE_OUTPUT:-$linux_share/sublime}"
capture_filter_b64="$(printf %s "${CAPTURE_FILTER:-}" | base64 | tr -d '\n')"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run capture-sublime.sh directly inside Linux" >&2
  exit 1
fi

# Keep the command line free of JSON and paths containing spaces. Parallels' Linux guest-tools
# argument handling does not preserve those reliably; the shared script does all substantive work.
exec prlctl exec "$vm" env \
  CAPTURE_FILTER_B64="$capture_filter_b64" \
  CAPTURE_LIMIT="${CAPTURE_LIMIT:-0}" \
  CAPTURE_TESTS="$capture_tests" \
  CAPTURE_OUTPUT="$capture_output" \
  LINUX_CAPTURE_BACKEND="${LINUX_CAPTURE_BACKEND:-auto}" \
  LINUX_CAPTURE_LOGICAL_TOP_INSET="${LINUX_CAPTURE_LOGICAL_TOP_INSET:-37}" \
  bash "$linux_share/capture-sublime.sh" "$@"
