#!/bin/bash
# Diffs every UI-conformance capture against its Sublime Text counterpart serially.
set -e
cd "$(dirname "$0")"

budget="${1:-200}"
diff_small="diff-ui-small"
diff_large="diff-ui-large"
correct="correct-ui.txt"

rm -rf "$diff_small" "$diff_large"
mkdir -p "$diff_small" "$diff_large"
rm -f "$correct"

images=(ours-ui/*.png)
if [[ ! -f "${images[0]}" ]]; then
  echo "no PNGs found under ours-ui/" >&2
  exit 2
fi

for ours in "${images[@]}"; do
  name=$(basename "$ours")
  reference="sublime-ui/$name"
  if [[ ! -f "$reference" ]]; then
    echo "missing reference: $reference" >&2
    exit 2
  fi

  temporary=$(mktemp "/tmp/ui-conformance-diff.XXXXXX")
  count=$(magick "$ours" "$reference" -alpha off -compose difference -composite \
    -threshold 0 -separate -evaluate-sequence max \
    -write "png:$temporary" \
    -format "%[fx:int(mean*w*h)]" info:)

  if [[ "$count" -eq 0 ]]; then
    rm -f "$temporary"
    printf "%s\n" "$name" >>"$correct"
  elif [[ "$count" -le "$budget" ]]; then
    mv "$temporary" "$diff_small/$name"
    printf "%6d px  %s\n" "$count" "$name" >>"$diff_small/diffs.txt"
  else
    mv "$temporary" "$diff_large/$name"
    printf "%6d px  %s\n" "$count" "$name" >>"$diff_large/diffs.txt"
  fi
done

touch "$diff_small/diffs.txt"
touch "$diff_large/diffs.txt"
touch "$correct"
sort -rn -o "$diff_small/diffs.txt" "$diff_small/diffs.txt" 2>/dev/null || true
sort -rn -o "$diff_large/diffs.txt" "$diff_large/diffs.txt" 2>/dev/null || true
sort -o "$correct" "$correct" 2>/dev/null || true

printf "%-12s %5d\n" "Correct:" "$(wc -l <"$correct")"
printf "%-12s %5d\n" "Small diffs:" "$(wc -l <"$diff_small/diffs.txt")"
printf "%-12s %5d\n" "Large diffs:" "$(wc -l <"$diff_large/diffs.txt")"
