#!/bin/bash
# Runs Linux backing-store capture in the Parallels VM from a macOS host.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Fedora 42 ARM64}"
linux_share="${LINUX_SHARE:-/media/psf/$(basename "$build_dir")}"
buffer_filter_b64="$(printf %s "${BUFFER_FILTER:-}" | base64 | tr -d '\n')"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run capture-ours.sh directly inside Linux" >&2
  exit 1
fi

# PX_USE_GL is opt-in: only forward it when actually set, since `env KEY=` with an
# empty value still defines KEY in the child environment (unlike leaving it unset),
# which would force the GL path on even for a default/software-path run.
env_args=(
  BUFFER_CROP="${BUFFER_CROP:-0,0,1600,600}"
  BUFFER_FILTER_B64="$buffer_filter_b64"
  BUFFER_HOLD_SECONDS="${BUFFER_HOLD_SECONDS:-0}"
  BUFFER_LIMIT="${BUFFER_LIMIT:-0}"
  LINUX_CAPTURE_TESTS="${LINUX_CAPTURE_TESTS:-$linux_share/capture-tests}"
  LINUX_OUR_OUTPUT="${LINUX_OUR_OUTPUT:-$linux_share/ours}"
)
if [[ -n "${PX_USE_GL:-}" ]]; then
  env_args+=(PX_USE_GL="$PX_USE_GL")
fi

exec prlctl exec "$vm" env "${env_args[@]}" bash "$linux_share/capture-ours.sh"
