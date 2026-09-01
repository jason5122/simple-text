#!/bin/bash
# Captures the configured sidebar shaping probes through platform_sidebar.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
out="$build_dir/ours-ui"
tests="$build_dir/capture-tests-ui"
crop="${1:-0,80,600,500}"

rm -rf "$out"
"$build_dir/platform_sidebar" --test "$tests" "$out" --crop "$crop"
