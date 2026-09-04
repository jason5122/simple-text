#!/bin/bash
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$build_dir/../.." && pwd)"
app="$repo_root/experiments/platform/conformance/Sublime Text.app"
subl="$app/Contents/SharedSupport/bin/subl"
tests="$build_dir/capture-tests"
out="${1:-$build_dir/sublime-metrics}"
limit="${METRICS_LIMIT:-0}"

if [[ ! -x "$subl" ]]; then
  echo "isolated Sublime Text not found at $app" >&2
  exit 1
fi

st_pid=$(pgrep -f "$app/Contents/MacOS/sublime_text" | sort -n | head -1)
if [[ -z "$st_pid" ]]; then
  echo "isolated Sublime Text not running (launch $app, then rerun this script)" >&2
  exit 1
fi

read_config() { grep -vE '^[[:space:]]*(#|$)' "$1" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*$//'; }
faces=()
while IFS= read -r line; do faces+=("$line"); done < <(read_config "$tests/faces-mac.txt")
sizes=()
while IFS= read -r line; do sizes+=("$line"); done < <(read_config "$tests/sizes.txt")
text_paths=("$tests"/texts/*.txt)
if [[ ${#faces[@]} -eq 0 || ${#sizes[@]} -eq 0 || ! -f "${text_paths[0]}" ]]; then
  echo "no faces, sizes, or text corpora under $tests" >&2
  exit 2
fi

rm -rf "$out"
mkdir -p "$out"
total=$((${#text_paths[@]} * ${#faces[@]} * ${#sizes[@]}))
current=0

for text_path in "${text_paths[@]}"; do
  stem=$(basename "$text_path" .txt)
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      output_path="$out/$stem-$face-$size.json"
      command="metrics_probe {\"text_path\":\"$text_path\",\"face\":\"$face\",\"size\":$size,\"output_path\":\"$output_path\"}"
      "$subl" --background --command "$command" </dev/null

      deadline=$((SECONDS + 20))
      while [[ ! -f "$output_path" && $SECONDS -lt $deadline ]]; do
        sleep 0.02
      done
      if [[ ! -f "$output_path" ]]; then
        echo "timed out waiting for $output_path" >&2
        exit 1
      fi
      if grep -q '"error"' "$output_path"; then
        echo "metrics probe failed: $output_path" >&2
        exit 1
      fi

      current=$((current + 1))
      printf '[%d/%d] %s\n' "$current" "$total" "$output_path"
      if [[ "$limit" -gt 0 && "$current" -ge "$limit" ]]; then
        exit 0
      fi
    done
  done
done
