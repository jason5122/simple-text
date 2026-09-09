#!/bin/sh
# Records your own scrolling in the editor, then reports on it and writes a replayable trace.
#
#   benchmark/record_editor_scroll.sh [name]
#
# Scroll the document. When a frame looks wrong, click in the text: that writes a mark. Quit with
# Cmd+Q (or Escape twice) when done. Start from the top of the document so a replay, which also
# starts there, hits the same clamps. Output goes to /tmp/editor-scroll-<name>.{log,tsv}; replay
# the trace with benchmark/replay_editor_scroll.sh.

set -eu

name="${1:-$(date +%Y%m%d-%H%M%S)}"
cd "$(dirname "$0")/.."
editor=out/release/editor
report=benchmark/scroll_log_report.py
log="/tmp/editor-scroll-$name.log"
trace="/tmp/editor-scroll-$name.tsv"

if [ ! -x "$editor" ]; then
  echo "record_editor_scroll: not built: $editor (bin/ninja -C out/release editor)" >&2
  exit 2
fi

echo "Scroll the document; click in the text to mark a bad frame; Cmd+Q when done."
EDITOR_SCROLL_TRACE=1 "$editor" 2> "$log" || true

python3 "$report" trace "$log" "$trace"
python3 "$report" report "$log"
echo
echo "log:    $log"
echo "trace:  $trace"
echo "replay: benchmark/replay_editor_scroll.sh $trace"
