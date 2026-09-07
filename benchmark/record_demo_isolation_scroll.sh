#!/bin/sh

set -eu

size="${1:-large}"
interval_us="${2:-4000}"
scroll_delta="${3:-48}"
mode="${4:-normal}"
content="${5:-all}"
output="${6:-}"

# Preserve the original four-argument form, where the fourth argument was the output path.
case "$mode" in
  *.mov)
    output=$mode
    mode=normal
    content=all
    ;;
esac

case "$content" in
  *.mov)
    output=$content
    content=all
    ;;
esac

usage() {
  echo "usage: $0 [small|large] [interval_us] [scroll_delta] [normal|onscreen|lower|offscreen|all] [all|all-cached|rectangles|text|text-cached] [output.mov]" >&2
}

case "$size" in
  small|large) ;;
  *)
    usage
    exit 2
    ;;
esac
case "$interval_us" in
  *[!0-9]*|'')
    usage
    exit 2
    ;;
esac
case "$scroll_delta" in
  *[!0-9]*|'')
    usage
    exit 2
    ;;
esac
case "$mode" in
  normal|onscreen|lower|offscreen) ;;
  all)
    if [ -n "$output" ]; then
      echo "record_demo_isolation_scroll: an output path cannot be used with mode 'all'" >&2
      exit 2
    fi
    for comparison_mode in offscreen onscreen normal; do
      "$0" "$size" "$interval_us" "$scroll_delta" "$comparison_mode" "$content"
    done
    exit 0
    ;;
  *)
    usage
    exit 2
    ;;
esac
case "$content" in
  all|all-cached|rectangles|text|text-cached) ;;
  *)
    usage
    exit 2
    ;;
esac

if [ -z "$output" ]; then
  output="/tmp/platform-demo-isolation-${mode}-${content}-${size}-${scroll_delta}px-${interval_us}us-$(date +%Y%m%d-%H%M%S).mov"
fi

cd "$(dirname "$0")/../../.."
repo_root=$PWD
benchmark="$repo_root/out/release/demo_isolation"
scroller="$repo_root/out/release/scroll_wheel"
analyzer="$repo_root/out/release/animation_analyzer"
log=$(mktemp /tmp/platform-demo-isolation.XXXXXX)

for executable in "$benchmark" "$scroller" "$analyzer"; do
  if [ ! -x "$executable" ]; then
    echo "record_demo_isolation_scroll: not built: $executable" >&2
    exit 2
  fi
done
if [ -e "$output" ]; then
  echo "record_demo_isolation_scroll: refusing to overwrite: $output" >&2
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

expected_scroll_distance=$((scroll_delta * 40 * 16))
"$benchmark" "$mode" "$size" "$expected_scroll_distance" "$content" >"$log" 2>&1 &
benchmark_pid=$!

tries=0
while ! grep -q 'capture_rect=' "$log"; do
  if ! kill -0 "$benchmark_pid" 2>/dev/null; then
    cat "$log" >&2
    exit 3
  fi
  tries=$((tries + 1))
  if [ "$tries" -ge 100 ]; then
    echo "record_demo_isolation_scroll: timed out waiting for benchmark window" >&2
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
  echo "record_demo_isolation_scroll: could not parse geometry: $geometry" >&2
  exit 3
fi
capture_x=$1
capture_y=$2
capture_w=$3
capture_h=$4
scroll_x=$((capture_x + capture_w / 2))
scroll_y=$((capture_y + capture_h * 3 / 4))

start_capture() {
  /usr/sbin/screencapture -v -C -x \
    -R"$capture_x,$capture_y,$capture_w,$capture_h" -V4 "$output" &
  capture_pid=$!
  sleep 0.65
  kill -0 "$capture_pid" 2>/dev/null
}

if ! start_capture; then
  wait "$capture_pid" 2>/dev/null || true
  capture_pid=
  rm -f "$output"
  echo "record_demo_isolation_scroll: retrying screen capture startup" >&2
  if ! start_capture; then
    echo "record_demo_isolation_scroll: screen capture failed to start" >&2
    exit 3
  fi
fi
"$scroller" "$scroll_x" "$scroll_y" "$scroll_delta" 40 "$interval_us" 8
tries=0
while ! grep -q 'scroll_input_complete' "$log"; do
  tries=$((tries + 1))
  if [ "$tries" -ge 40 ]; then
    echo "record_demo_isolation_scroll: benchmark did not receive the complete scroll input" >&2
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

echo "content_mode=$mode"
echo "content_kind=$content"
cat "$log"
echo "$output"
"$analyzer" --input "$output" --points-width "$capture_w" --speed 720 \
  --start 0.55 --duration 2.8 --top-points 96 --wrap-points "$((capture_w - 120))"
