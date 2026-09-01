#!/bin/bash
# Screenshots every tests/texts x faces x sizes combination into the build's ours/ directory.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
tests="$build_dir/capture-tests"
out="$build_dir/ours"
crop="${1:-0,2,1600,600}"

rm -rf "$out"
"$build_dir/platform_rasterizer" --test "$tests" "$out" --crop "$crop"
