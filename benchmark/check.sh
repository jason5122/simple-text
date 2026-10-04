#!/bin/sh
# Replays the checked-in scroll and drag traces through the benchmarks and prints the lines that
# say whether scrolling regressed. Both runs open a window for about five seconds; keep hands off
# the trackpad meanwhile, and check `top -o cpu` for WindowServer first. benchmark/README.md,
# "Regression check", says what the lines should read.
#
#   benchmark/check.sh            out/release
#   BUILD=out/debug benchmark/check.sh
set -e
cd "$(dirname "$0")/.."
BUILD=${BUILD:-out/release}

# The window server runs at half rate in Low Power Mode, which changes every number below, so
# runs are comparable only within one power state (pmset's key is powermode or lowpowermode by
# OS version; 0 is normal). The drag's `refresh=` and the flick's `presentation_interval` show
# the rate the run actually got.
echo "== power: $(pmset -g 2>/dev/null | grep -i powermode | tr -s ' ' | sed 's/^ //')"

run() {
    name=$1
    shift
    echo "== $name"
    "$@" |
        grep -E '^(benchmark|presentation_interval|input_to_present|presented_step_error|step_deviation|summary)'
}

run scroll "$BUILD/scroll_benchmark" --replay benchmark/traces/flick.tsv
run drag "$BUILD/scrollbar_benchmark" --replay benchmark/traces/drag.tsv
