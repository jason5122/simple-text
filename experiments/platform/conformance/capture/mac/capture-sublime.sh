#!/bin/bash
# Captures the Sublime Text reference suite into the build's sublime/ directory.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$build_dir/../.." && pwd)"
app="$repo_root/experiments/platform/conformance/Sublime Text.app"
subl="$app/Contents/SharedSupport/bin/subl"
server="$build_dir/platform_capture_server"
out="$build_dir/sublime"
crop="${1:-0,58,1600,600}"

if [[ ! -x "$subl" ]]; then
  echo "isolated Sublime Text not found at $app" >&2
  echo "put it at '$app'" >&2
  exit 1
fi

st_pid=$(pgrep -f "$app/Contents/MacOS/sublime_text" | sort -n | head -1)
if [[ -z "$st_pid" ]]; then
  echo "isolated Sublime Text not running (launch $app, then rerun this script)" >&2
  exit 1
fi

# One trimmed value per line, skipping blank and '#'-comment lines.
read_config() { grep -vE '^[[:space:]]*(#|$)' "$1" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*$//'; }
tests="$build_dir/capture-tests"
faces=()
while IFS= read -r line; do faces+=("$line"); done < <(read_config "$tests/faces-mac.txt")
sizes=()
while IFS= read -r line; do sizes+=("$line"); done < <(read_config "$tests/sizes.txt")
text_paths=("$tests"/texts/*.txt)
if [[ ${#faces[@]} -eq 0 || ${#sizes[@]} -eq 0 || ! -f "${text_paths[0]}" ]]; then
  echo "no faces, sizes, or text corpora under $tests" >&2
  exit 2
fi
total=$((${#text_paths[@]} * ${#faces[@]} * ${#sizes[@]}))

rm -rf "$out"
mkdir -p "$out"

# Drive one capture server over FIFOs (bash 3.2 has no coproc). Text is the outermost loop: a
# corpus change is the costliest step, so it happens least often.
request_fifo=$(mktemp -u)
response_fifo=$(mktemp -u)
mkfifo "$request_fifo" "$response_fifo"
"$server" --pid "$st_pid" --crop "$crop" <"$request_fifo" >"$response_fifo" &
server_pid=$!
exec 3>"$request_fifo"
exec 4<"$response_fifo"

fails=0
current=0
for text_path in "${text_paths[@]}"; do
  stem=$(basename "$text_path" .txt)
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      "$subl" --background --command \
        "rasterizer_render {\"text_path\": \"$text_path\", \"face\": \"$face\", \"size\": $size}" </dev/null
      current=$((current + 1))
      printf '[%d/%d] ' "$current" "$total" >&2
      echo "$out/$stem-$face-$size.png" >&3
      read -r reply <&4
      case "$reply" in err*) fails=$((fails + 1)) ;; esac
    done
  done
done

echo quit >&3
exec 3>&-
wait "$server_pid"
rm -f "$request_fifo" "$response_fifo"
if [[ "$fails" -ne 0 ]]; then
  echo "$fails shot(s) failed" >&2
  exit 1
fi
