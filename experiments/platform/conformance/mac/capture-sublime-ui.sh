#!/bin/bash
# Captures the configured sidebar shaping probes from an isolated Sublime Text instance.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
repo_root="$(cd "$build_dir/../.." && pwd)"
app="$repo_root/experiments/platform/conformance/Sublime Text.app"
subl="$app/Contents/SharedSupport/bin/subl"
server="$build_dir/capture_server"
out="$build_dir/sublime-ui"
tests="$build_dir/capture-tests-ui"
crop="${1:-0,80,600,500}"

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

rm -rf "$out"
mkdir -p "$out"

stems=()
commands=()
read_config() { grep -vE '^[[:space:]]*(#|$)' "$1" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*$//'; }
faces=()
while IFS= read -r line; do faces+=("$line"); done < <(read_config "$tests/faces.txt")
sizes=()
while IFS= read -r line; do sizes+=("$line"); done < <(read_config "$tests/sizes.txt")
text_paths=("$tests"/texts/*.txt)
if [[ ${#faces[@]} -eq 0 || ${#sizes[@]} -eq 0 || ! -f "${text_paths[0]}" ]]; then
  echo "no UI test inputs found under $tests" >&2
  exit 2
fi
for text_path in "${text_paths[@]}"; do
  corpus=$(basename "$text_path" .txt)
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      stems+=("$corpus-$face-$size")
      commands+=(
        "sidebar_render {\"text_path\":\"$text_path\",\"face\":\"$face\",\"size\":$size}"
      )
    done
  done
done

request_fifo=$(mktemp -u)
response_fifo=$(mktemp -u)
baseline=$(mktemp -t sublime-ui-baseline)
mkfifo "$request_fifo" "$response_fifo"
"$server" --pid "$st_pid" --crop "$crop" <"$request_fifo" >"$response_fifo" &
server_pid=$!
exec 3>"$request_fifo"
exec 4<"$response_fifo"

cleanup() {
  rm -f "$request_fifo" "$response_fifo" "$baseline"
}
trap cleanup EXIT

echo "$baseline" >&3
read -r reply <&4
case "$reply" in
  err*) exit 1 ;;
esac

fails=0
for ((i = 0; i < ${#commands[@]}; i++)); do
  "$subl" --background --command "${commands[$i]}" </dev/null
  # The project label changes before Sublime reloads the generated theme. Consume that frame so
  # the named capture waits for the font change rather than recording the previous case's face.
  echo "$baseline" >&3
  read -r reply <&4
  case "$reply" in
    err*) fails=$((fails + 1)) ;;
  esac
  printf '[%d/%d] ' "$((i + 1))" "${#commands[@]}" >&2
  echo "$out/${stems[$i]}.png" >&3
  read -r reply <&4
  case "$reply" in
    err*) fails=$((fails + 1)) ;;
  esac
done

echo quit >&3
exec 3>&-
wait "$server_pid"
if [[ "$fails" -ne 0 ]]; then
  echo "$fails shot(s) failed" >&2
  exit 1
fi
