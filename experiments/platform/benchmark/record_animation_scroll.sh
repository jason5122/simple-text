#!/bin/sh

set -eu

size="${1:-large}"
interval_us="${2:-4000}"
rectangle_count="${3:-0}"
output="${4:-/tmp/platform-animation-${size}-${rectangle_count}rects-${interval_us}us-$(date +%Y%m%d-%H%M%S).mov}"

case "$size" in
  small|large) ;;
  *)
    echo "usage: $0 [small|large] [interval_us] [rectangle_count] [output.mov]" >&2
    exit 2
    ;;
esac
case "$interval_us" in
  *[!0-9]*|'')
    echo "usage: $0 [small|large] [interval_us] [rectangle_count] [output.mov]" >&2
    exit 2
    ;;
esac
case "$rectangle_count" in
  *[!0-9]*|'')
    echo "usage: $0 [small|large] [interval_us] [rectangle_count] [output.mov]" >&2
    exit 2
    ;;
esac

cd "$(dirname "$0")/../../.."
repo_root=$PWD
benchmark="$repo_root/out/release/platform_animation_benchmark"
scroller="$repo_root/out/release/scroll_wheel"
analyzer="$repo_root/out/release/platform_animation_analyzer"
log=$(mktemp /tmp/platform-animation-benchmark.XXXXXX)

for executable in "$benchmark" "$scroller" "$analyzer"; do
  if [ ! -x "$executable" ]; then
    echo "record_animation_scroll: not built: $executable" >&2
    exit 2
  fi
done
if [ -e "$output" ]; then
  echo "record_animation_scroll: refusing to overwrite: $output" >&2
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

"$benchmark" "$size" "$rectangle_count" >"$log" 2>&1 &
benchmark_pid=$!

tries=0
while ! grep -q 'capture_rect=' "$log"; do
  if ! kill -0 "$benchmark_pid" 2>/dev/null; then
    cat "$log" >&2
    exit 3
  fi
  tries=$((tries + 1))
  if [ "$tries" -ge 100 ]; then
    echo "record_animation_scroll: timed out waiting for benchmark window" >&2
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
  echo "record_animation_scroll: could not parse geometry: $geometry" >&2
  exit 3
fi
capture_x=$1
capture_y=$2
capture_w=$3
capture_h=$4
scroll_x=$((capture_x + capture_w / 2))
scroll_y=$((capture_y + capture_h / 2))

/usr/sbin/screencapture -v -C -x \
  -R"$capture_x,$capture_y,$capture_w,$capture_h" -V4 "$output" &
capture_pid=$!
sleep 0.65
"$scroller" "$scroll_x" "$scroll_y" 12 40 "$interval_us" 8
tries=0
while ! grep -q 'scroll_input_complete' "$log"; do
  tries=$((tries + 1))
  if [ "$tries" -ge 40 ]; then
    echo "record_animation_scroll: benchmark did not receive the complete scroll input" >&2
    cat "$log" >&2
    exit 3
  fi
  sleep 0.05
done
wait "$capture_pid"
capture_pid=

kill "$benchmark_pid" 2>/dev/null || true
wait "$benchmark_pid" 2>/dev/null || true
benchmark_pid=

cat "$log"
echo "$output"
"$analyzer" --input "$output" --points-width "$capture_w" --speed 120 \
  --start 0.55 --duration 2.8
