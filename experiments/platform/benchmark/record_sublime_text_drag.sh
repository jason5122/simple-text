#!/bin/sh

# Foreground Sublime Text, record its sidebar, drive the same synthetic drag as the Platform
# benchmark, and emit the same analyzer summary for direct comparison.

set -eu

interval_us="${1:-200}"
output="${2:-/tmp/sublime-text-drag-${interval_us}us-$(date +%Y%m%d-%H%M%S).mov}"

case "$interval_us" in
  *[!0-9]*|'')
    echo "usage: $0 [interval_us] [output.mov]" >&2
    exit 2
    ;;
esac

cd "$(dirname "$0")/../../.."
repo_root=$PWD
mover="$repo_root/out/release/move_mouse"
analyzer="$repo_root/out/release/platform_benchmark_analyzer"
app="${SUBLIME_TEXT_APP:-/Applications/Sublime Text.app}"

if [ ! -x "$mover" ]; then
  echo "record_sublime_text_drag: mover is not executable: $mover" >&2
  exit 2
fi
if [ ! -x "$analyzer" ]; then
  echo "record_sublime_text_drag: analyzer is not built: $analyzer" >&2
  exit 2
fi
if [ ! -d "$app" ]; then
  echo "record_sublime_text_drag: application is missing: $app" >&2
  exit 2
fi
if [ -e "$output" ]; then
  echo "record_sublime_text_drag: refusing to overwrite: $output" >&2
  exit 2
fi

capture_pid=
cleanup() {
  if [ -n "$capture_pid" ] && kill -0 "$capture_pid" 2>/dev/null; then
    kill "$capture_pid" 2>/dev/null || true
    wait "$capture_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT HUP INT TERM

# This is the original reproducible sidebar-folder test. Its fixed crop and coordinates require
# a folder row at (80, 130), as described in README.md.
open -a "$app"
sleep 0.35
/usr/sbin/screencapture -v -C -x -R0,80,420,800 -V4 "$output" &
capture_pid=$!
sleep 0.65
"$mover" 80 130 80 800 200 "$interval_us" 10 --drag
wait "$capture_pid"
capture_pid=

echo "$output"
"$analyzer" --mode sublime --input "$output" --points-width 420
