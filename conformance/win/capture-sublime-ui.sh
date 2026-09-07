#!/bin/bash
# Captures native Windows Sublime Text sidebar references through Parallels.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
vm="${PARALLELS_VM:-Windows 11}"
windows_share="${WINDOWS_SHARE:-\\\\Mac\\$(basename "$build_dir")}"
crop="${1:-0,0,600,500}"

if ! command -v prlctl >/dev/null; then
  echo "prlctl not found; run the PowerShell script directly inside Windows" >&2
  exit 1
fi

rm -rf "$build_dir/sublime-ui"
prlctl exec "$vm" --current-user \
  powershell.exe -NoProfile -ExecutionPolicy Bypass \
  -File "$windows_share\\capture-sublime-ui.ps1" \
  -CaptureExecutable "$windows_share\\capture_windows.exe" \
  -TestsDirectory "$windows_share\\capture-tests-ui" \
  -OutputDirectory "$windows_share\\sublime-ui" \
  -PluginPath "$windows_share\\sidebar_render.py" \
  -Crop "$crop"
