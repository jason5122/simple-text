#!/bin/bash
set -e

BUILD_DIR="$1"
cd "$BUILD_DIR"
ninja

TMP_DIR="$(mktemp -d -t flamegraph)"
trap 'rm -rf "$TMP_DIR"' EXIT

TRACE="$TMP_DIR/flamegraph.trace"
XML="$TMP_DIR/flamegraph.xml"
FOLDED="$TMP_DIR/stacks.folded"

# # We use `--attach` because `--launch` seems to double-launch the app.
# # xctrace record --template 'Time Profiler' --output "$TRACE" --launch -- "Simple Text.app"

open -n "./Simple Text.app"
xctrace record --template 'Time Profiler' --output "$TRACE" --attach "Simple Text"
xctrace export --input "$TRACE" --xpath '/trace-toc/*/data/table[@schema="time-profile"]' --output "$XML"

inferno-collapse-xctrace "$XML" >"$FOLDED"
inferno-flamegraph --truncate-text-right "$FOLDED" >flamegraph.svg
