#!/bin/sh

# Launch, foreground, record, drive, and analyze the platform drag benchmark as one
# atomic job. The benchmark prints its content rectangle, so this remains correct if macOS centers
# it at a different screen coordinate.

set -eu

interval_us="${1:-200}"
output="${2:-/tmp/platform-drag-${interval_us}us-$(date +%Y%m%d-%H%M%S).mov}"

case "$interval_us" in
  *[!0-9]*|'')
    echo "usage: $0 [interval_us] [output.mov]" >&2
    exit 2
    ;;
esac

cd "$(dirname "$0")/../../.."
repo_root=$PWD
benchmark="$repo_root/out/release/platform_benchmark"
mover="$repo_root/out/release/move_mouse"
analyzer="$repo_root/out/release/platform_benchmark_analyzer"
log=$(mktemp /tmp/platform-drag-benchmark.XXXXXX)

if [ ! -x "$benchmark" ]; then
  echo "record_platform_drag: benchmark is not built: $benchmark" >&2
  exit 2
fi
if [ ! -x "$mover" ]; then
  echo "record_platform_drag: mover is not executable: $mover" >&2
  exit 2
fi
if [ ! -x "$analyzer" ]; then
  echo "record_platform_drag: analyzer is not built: $analyzer" >&2
  exit 2
fi
if [ -e "$output" ]; then
  echo "record_platform_drag: refusing to overwrite: $output" >&2
  exit 2
fi

benchmark_pid=
capture_pid=
cleanup() {
  if [ -n "$capture_pid" ] && kill -0 "$capture_pid" 2>/dev/null; then
    kill "$capture_pid" 2>/dev/null || true
    wait "$capture_pid" 2>/dev/null || true
  fi
  if [ -n "$benchmark_pid" ] && kill -0 "$benchmark_pid" 2>/dev/null; then
    kill "$benchmark_pid" 2>/dev/null || true
    wait "$benchmark_pid" 2>/dev/null || true
  fi
  rm -f "$log"
}
trap cleanup EXIT HUP INT TERM

PX_LAG_TRACE=1 PX_LAG_TRACE_SAMPLES=512 \
  "$benchmark" >"$log" 2>&1 &
benchmark_pid=$!

# Wait for px_show_window() and the geometry print. This also leaves enough time for the first
# complete backing-store paint before mouse injection begins.
tries=0
while ! grep -q 'capture_rect=' "$log"; do
  if ! kill -0 "$benchmark_pid" 2>/dev/null; then
    cat "$log" >&2
    exit 3
  fi
  tries=$((tries + 1))
  if [ "$tries" -ge 100 ]; then
    echo "record_platform_drag: timed out waiting for benchmark window" >&2
    cat "$log" >&2
    exit 3
  fi
  sleep 0.05
done
sleep 0.35

geometry=$(sed -n 's/.*capture_rect=//p' "$log" | sed -n '1p')
old_ifs=$IFS
IFS=,
set -- $geometry
IFS=$old_ifs
if [ "$#" -ne 4 ]; then
  echo "record_platform_drag: could not parse geometry: $geometry" >&2
  exit 3
fi
capture_x=$1
capture_y=$2
capture_w=$3
capture_h=$4
mouse_x=$((capture_x + capture_w / 2))
mouse_y0=$((capture_y + 50))
mouse_y1=$((capture_y + capture_h - 50))

/usr/sbin/screencapture -v -C -x \
  -R"$capture_x,$capture_y,$capture_w,$capture_h" -V4 "$output" &
capture_pid=$!
sleep 0.65
"$mover" "$mouse_x" "$mouse_y0" "$mouse_x" "$mouse_y1" \
  200 "$interval_us" 10 --drag
wait "$capture_pid"
capture_pid=

kill "$benchmark_pid" 2>/dev/null || true
wait "$benchmark_pid" 2>/dev/null || true
benchmark_pid=

cat "$log"
echo "$output"
"$analyzer" --mode platform --input "$output" --points-width "$capture_w"
