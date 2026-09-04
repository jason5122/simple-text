#!/bin/bash
set -e

vm="${PARALLELS_VM:-Windows 11}"
windows_share="${WINDOWS_SHARE:-\\\\Mac\\win-arm64}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run this script from the macOS host" >&2
  exit 1
fi

prlctl exec "$vm" --current-user \
  powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$windows_share\\capture-sublime-metrics.ps1" \
  -TestsDirectory "$windows_share\\capture-tests" \
  -OutputDirectory "$windows_share\\sublime-metrics" \
  -PluginPath "$windows_share\\metrics_probe.py" \
  -Limit "${METRICS_LIMIT:-0}"
