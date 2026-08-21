#!/bin/bash
# Diffs every ours/*.png against its sublime/ counterpart in parallel (each pair is independent --
# read two files, diff, write a result -- so this fans out across cores with no shared state besides
# appending short lines to the two log files, which is safe: POSIX guarantees an append below
# PIPE_BUF -- our lines are ~20 bytes -- doesn't interleave between processes). See diff-one.sh for
# the per-pair work and why it's one process instead of two.
set -e

cd "$(dirname "$0")"

BUDGET="${1:-200}"
JOBS="${2:-$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || sysctl -n hw.ncpu)}"

DIFF_SMALL="diff-small"
DIFF_LARGE="diff-large"
CORRECT="correct.txt"

rm -rf "$DIFF_SMALL" "$DIFF_LARGE"
mkdir -p "$DIFF_SMALL" "$DIFF_LARGE"
rm -f "$CORRECT"

ls ours | xargs -P "$JOBS" -I{} ./diff-one.sh {} "$BUDGET"

# Sort each bucket's log by pixel count, greatest first.
touch "$DIFF_SMALL/diffs.txt"
touch "$DIFF_LARGE/diffs.txt"
touch "$CORRECT"
sort -rn -o "$DIFF_SMALL/diffs.txt" "$DIFF_SMALL/diffs.txt" 2>/dev/null || true
sort -rn -o "$DIFF_LARGE/diffs.txt" "$DIFF_LARGE/diffs.txt" 2>/dev/null || true
sort -o "$CORRECT" "$CORRECT" 2>/dev/null || true

printf "%-12s %5d\n" "Correct:" "$(wc -l <"$CORRECT")"
printf "%-12s %5d\n" "Small diffs:" "$(wc -l <"$DIFF_SMALL/diffs.txt")"
printf "%-12s %5d\n" "Large diffs:" "$(wc -l <"$DIFF_LARGE/diffs.txt")"
