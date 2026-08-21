#!/bin/bash
# Captures the isolated Sublime Text's reference render of every tests/texts x faces.txt x sizes.txt
# combo into sublime/, to diff against the rasterizer's own ours/ (see screenshot.sh, then test.sh).
#
# One long-lived capture_server holds a single WindowServer connection for the whole run (per-shot
# capture processes churn connections and stall). Bash drives the isolated ST via `subl --command`
# (the rasterizer_render.py plugin, which applies each combo synchronously) and feeds each output
# path to the server, which grabs the window once it settles. The isolated ST must already be running
# with rasterizer_render.py installed.
set -e

cd "$(dirname "$0")"

subl="./Sublime Text.app/Contents/SharedSupport/bin/subl"
server=../../out/release/capture_server
out=sublime

# Device-pixel, window-relative crop framing the same text region as the rasterizer's --test crop
# (which sits 56px higher -- a borderless window has no tab bar). Tune for the isolated ST window;
# pass as $1.
crop="0,136,1600,600"

(cd ../.. && ./bin/ninja -C out/release capture_server)

st_pid=$(pgrep -f "$PWD/Sublime Text.app/Contents/MacOS/sublime_text" | sort -n | head -1)
[ -n "$st_pid" ] || {
  echo "isolated Sublime Text not running (launch it with rasterizer_render.py)" >&2
  exit 1
}

# One trimmed value per line, skipping blank and '#'-comment lines.
read_config() { grep -vE '^[[:space:]]*(#|$)' "$1" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*$//'; }
faces=()
while IFS= read -r l; do faces+=("$l"); done < <(read_config tests/faces.txt)
sizes=()
while IFS= read -r l; do sizes+=("$l"); done < <(read_config tests/sizes.txt)
[ ${#faces[@]} -gt 0 ] && [ ${#sizes[@]} -gt 0 ] || {
  echo "no faces/sizes under tests/" >&2
  exit 2
}

rm -rf "$out"
mkdir -p "$out"

# Drive the one server over FIFOs (bash 3.2 has no coproc). Text is the outermost loop: a corpus
# change is the costliest step, so it happens least often.
req=$(mktemp -u)
resp=$(mktemp -u)
mkfifo "$req" "$resp"
"$server" --pid "$st_pid" --crop "$crop" <"$req" >"$resp" &
srv=$!
exec 3>"$req"
exec 4<"$resp"

fails=0
for txt in tests/texts/*.txt; do
  stem=$(basename "$txt" .txt)
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      "$subl" --background --command \
        "rasterizer_render {\"text_path\": \"$PWD/$txt\", \"face\": \"$face\", \"size\": $size}" </dev/null
      echo "$out/$stem-$face-$size.png" >&3
      read -r reply <&4
      case "$reply" in err*) fails=$((fails + 1)) ;; esac
    done
  done
done

echo quit >&3
exec 3>&-
wait "$srv"
rm -f "$req" "$resp"
[ "$fails" -eq 0 ] || {
  echo "$fails shot(s) failed" >&2
  exit 1
}
