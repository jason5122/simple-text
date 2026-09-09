#!/bin/sh
set -eu

if [ "$#" -lt 2 ]; then
    echo "usage: $0 OUT_DIR TRACE.tsv [scroll_benchmark options...]" >&2
    exit 2
fi

out_dir=$1
trace=$2
shift 2

echo "=== current event-driven scrolling ==="
"$out_dir/scroll_benchmark" "$trace" --mode event "$@"
echo
echo "=== display-aligned resampling ==="
"$out_dir/scroll_benchmark" "$trace" --mode resampled "$@"
