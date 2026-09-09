#!/bin/sh
# Replays a recorded scroll trace into a fresh editor and reports on the frames it produced,
# around the same marks as the recording.
#
#   benchmark/replay_editor_scroll.sh TRACE.tsv [name]
#
# The events are posted at the HID tap with their recorded timestamps, so leave the pointer alone
# and keep your hands off the trackpad while it runs. Needs Accessibility access for the terminal
# (System Settings > Privacy & Security), as scroll_trace replay does.

set -eu

if [ "$#" -lt 1 ]; then
  echo "usage: $0 TRACE.tsv [name]" >&2
  exit 2
fi
trace=$1
name="${2:-replay-$(date +%Y%m%d-%H%M%S)}"
cd "$(dirname "$0")/.."
editor=out/release/editor
replayer=out/release/scroll_trace
mouse=out/release/move_mouse
report=benchmark/scroll_log_report.py
log="/tmp/editor-scroll-$name.log"

for executable in "$editor" "$replayer" "$mouse"; do
  if [ ! -x "$executable" ]; then
    echo "replay_editor_scroll: not built: $executable" >&2
    exit 2
  fi
done
if [ ! -f "$trace" ]; then
  echo "replay_editor_scroll: no such trace: $trace" >&2
  exit 2
fi

duration=$(grep -v '^#' "$trace" | tail -1 | cut -f1)
duration_s=$(( duration / 1000000000 + 2 ))

EDITOR_SCROLL_TRACE=1 "$editor" 2> "$log" &
editor_pid=$!
cleanup() {
  if kill -0 "$editor_pid" 2>/dev/null; then
    kill "$editor_pid" 2>/dev/null || true
    wait "$editor_pid" 2>/dev/null || true
  fi
}
trap cleanup EXIT HUP INT TERM

sleep 2
# The editor opens maximized, so a point this far in is over the document on any display.
"$mouse" 900 600 900 600 2 20000 1 > /dev/null 2>&1
"$replayer" replay "$trace" "$editor_pid" 2>&1 | grep -v 'put the pointer' || true
sleep "$duration_s"
cleanup
trap - EXIT

python3 "$report" report "$log" --trace "$trace"
echo
echo "log: $log"
