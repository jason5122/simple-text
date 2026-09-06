#!/bin/bash
# Diffs every buffer-conformance capture against its Sublime Text counterpart.
set -e

cd "$(dirname "$0")"
script="$PWD/$(basename "$0")"
ours_dir="${OURS_DIR:-ours}"
reference_dir="${SUBLIME_DIR:-sublime}"
results_dir="${DIFF_RESULTS_DIR:-.}"
threshold="${DIFF_THRESHOLD:-0}"

case "$ours_dir" in /*) ;; *) ours_dir="$PWD/$ours_dir" ;; esac
case "$reference_dir" in /*) ;; *) reference_dir="$PWD/$reference_dir" ;; esac
case "$results_dir" in /*) ;; *) results_dir="$PWD/$results_dir" ;; esac

diff_one() {
  name="$1"
  budget="$2"
  ours="$ours_dir/$name"
  reference="$reference_dir/$name"

  if [[ ! -f "$reference" ]]; then
    echo "missing reference: $reference" >&2
    return 2
  fi

  # The PNG coder prefix is needed because BSD mktemp only substitutes trailing Xs.
  temporary=$(mktemp "/tmp/buffer-conformance-diff.XXXXXX")
  trap 'rm -f "$temporary"' EXIT
  count=$(magick "$ours" "$reference" -alpha off -compose difference -composite \
    -threshold "$threshold%" -separate -evaluate-sequence max \
    -write "png:$temporary" \
    -format "%[fx:int(mean*w*h)]" info:)

  if [[ "$count" -eq 0 ]]; then
    printf "%s\n" "$name" >>"$results_dir/correct.txt"
  elif [[ "$count" -le "$budget" ]]; then
    mv "$temporary" "$results_dir/diff-small/$name"
    temporary=""
    printf "%6d px  %s\n" "$count" "$name" >>"$results_dir/diff-small/diffs.txt"
  else
    mv "$temporary" "$results_dir/diff-large/$name"
    temporary=""
    printf "%6d px  %s\n" "$count" "$name" >>"$results_dir/diff-large/diffs.txt"
  fi
}

if [[ "${1:-}" == "--one" ]]; then
  diff_one "$2" "$3"
  exit
fi

budget="${1:-200}"
jobs="${2:-$(sysctl -n hw.perflevel0.physicalcpu 2>/dev/null || \
  sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}"

if ! command -v magick >/dev/null; then
  echo "ImageMagick's magick command is required" >&2
  exit 2
fi
images=("$ours_dir"/*.png)
if [[ ! -f "${images[0]}" ]]; then
  echo "no PNGs found under ours/" >&2
  exit 2
fi
if [[ ! -d "$reference_dir" ]]; then
  echo "no reference directory found at $reference_dir" >&2
  exit 2
fi

rm -rf "$results_dir/diff-small" "$results_dir/diff-large"
mkdir -p "$results_dir/diff-small" "$results_dir/diff-large"
rm -f "$results_dir/correct.txt"

# Each result line is shorter than POSIX PIPE_BUF, so parallel workers can append safely.
find "$ours_dir" -maxdepth 1 -type f -name '*.png' -exec basename {} \; | \
  xargs -P "$jobs" -I{} "$script" --one {} "$budget"

touch "$results_dir/diff-small/diffs.txt" "$results_dir/diff-large/diffs.txt" \
  "$results_dir/correct.txt"
sort -rn -o "$results_dir/diff-small/diffs.txt" "$results_dir/diff-small/diffs.txt" \
  2>/dev/null || true
sort -rn -o "$results_dir/diff-large/diffs.txt" "$results_dir/diff-large/diffs.txt" \
  2>/dev/null || true
sort -o "$results_dir/correct.txt" "$results_dir/correct.txt" 2>/dev/null || true

printf "%-12s %5d\n" "Correct:" "$(wc -l <"$results_dir/correct.txt")"
printf "%-12s %5d\n" "Small diffs:" \
  "$(wc -l <"$results_dir/diff-small/diffs.txt")"
printf "%-12s %5d\n" "Large diffs:" \
  "$(wc -l <"$results_dir/diff-large/diffs.txt")"
