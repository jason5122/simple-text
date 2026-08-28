#!/bin/bash
# Screenshots the rasterizer across every tests/texts x faces x sizes combo, into ours/. One
# persistent window renders all combos and captures itself -- no per-shot relaunch, no fixed delays.
# Pair with capture-sublime.sh (writes sublime/), then test.sh.
set -e

cd "$(dirname "$0")/../.."

./bin/ninja -C out/release

out_dir="experiments/rasterizer/ours"
rm -rf "$out_dir"

# --crop (device pixels, top-left origin) frames the text region; pass as $1 to override the binary's
# built-in default. It must match capture-sublime.sh's ST crop shifted up 56px (no tab bar here).
./out/release/platform_rasterizer --test experiments/rasterizer/tests "$out_dir" --crop 0,80,1600,600
