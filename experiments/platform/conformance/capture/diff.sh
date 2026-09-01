#!/bin/bash
set -e

cd "$(dirname "$0")"
script="$PWD/$(basename "$0")"

diff_one() {
  name="$1"
  budget="$2"
  ours="ours/$name"
  reference="sublime/$name"

  if [[ ! -f "$reference" ]]; then
    echo "missing reference: $reference" >&2
    return 2
  fi

  # The PNG coder prefix is needed because BSD mktemp only substitutes trailing Xs.
  temporary=$(mktemp "/tmp/platform-rasterizer-diff.XXXXXX")
  trap 'rm -f "$temporary"' EXIT
  count=$(magick "$ours" "$reference" -alpha off -compose difference -composite \
    -threshold 0 -separate -evaluate-sequence max \
    -write "png:$temporary" \
    -format "%[fx:int(mean*w*h)]" info:)

  if [[ "$count" -eq 0 ]]; then
    printf "%s\n" "$name" >>correct.txt
  elif [[ "$count" -le "$budget" ]]; then
    mv "$temporary" "diff-small/$name"
    temporary=""
    printf "%6d px  %s\n" "$count" "$name" >>diff-small/diffs.txt
  else
    mv "$temporary" "diff-large/$name"
    temporary=""
    printf "%6d px  %s\n" "$count" "$name" >>diff-large/diffs.txt
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
images=(ours/*.png)
if [[ ! -f "${images[0]}" ]]; then
  echo "no PNGs found under ours/" >&2
  exit 2
fi
if [[ ! -d sublime ]]; then
  echo "no sublime/ reference directory found" >&2
  exit 2
fi

rm -rf diff-small diff-large
mkdir -p diff-small diff-large
rm -f correct.txt

# Each result line is shorter than POSIX PIPE_BUF, so parallel workers can append safely.
find ours -maxdepth 1 -type f -name '*.png' -exec basename {} \; | \
  xargs -P "$jobs" -I{} "$script" --one {} "$budget"

touch diff-small/diffs.txt diff-large/diffs.txt correct.txt
sort -rn -o diff-small/diffs.txt diff-small/diffs.txt 2>/dev/null || true
sort -rn -o diff-large/diffs.txt diff-large/diffs.txt 2>/dev/null || true
sort -o correct.txt correct.txt 2>/dev/null || true

printf "%-12s %5d\n" "Correct:" "$(wc -l <correct.txt)"
printf "%-12s %5d\n" "Small diffs:" "$(wc -l <diff-small/diffs.txt)"
printf "%-12s %5d\n" "Large diffs:" "$(wc -l <diff-large/diffs.txt)"
