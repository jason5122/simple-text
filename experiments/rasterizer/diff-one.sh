#!/bin/bash
# Diffs one ours/sublime pair and files it into diff-small/ or diff-large/ by pixel budget. Called
# by diff.sh, one process per image, fanned out via xargs -P -- see diff.sh for why.
set -e
cd "$(dirname "$0")"

name="$1"
budget="$2"
a="ours/$name"
b="sublime/$name"

# No suffix: BSD mktemp only substitutes a trailing run of X's, so a template like foo.XXXXXX.png is
# taken as a literal filename (every parallel job would collide on the exact same path). Force PNG
# via the png: coder prefix on the -write target instead of relying on the extension.
tmp=$(mktemp "/tmp/rzdiff.XXXXXX")

# One magick invocation does both jobs at once: -write mid-pipeline saves the thresholded diff mask
# (a crisp white-on-black map of every differing pixel -- more legible than a raw magnitude image for
# subtle diffs, and it's already computed for the count) without stopping the pipe, which continues on
# to print the pixel count. This replaces two full decode+composite passes (a magick count call plus a
# separate `compare` call) with one.
count=$(magick "$a" "$b" -alpha off -compose difference -composite \
  -threshold 0 -separate -evaluate-sequence max \
  -write "png:$tmp" \
  -format "%[fx:int(mean*w*h)]" info:)

if [[ "$count" -eq 0 ]]; then
  rm -f "$tmp"
  printf "%s\n" "$name" >>correct.txt
elif [[ "$count" -le "$budget" ]]; then
  mv "$tmp" "diff-small/$name"
  printf "%6d px  %s\n" "$count" "$name" >>diff-small/diffs.txt
else
  mv "$tmp" "diff-large/$name"
  printf "%6d px  %s\n" "$count" "$name" >>diff-large/diffs.txt
fi
