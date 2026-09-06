#!/bin/bash
# Collects reference layout metrics from a running Linux Sublime Text instance.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"

# prlctl enters the guest as root. Re-enter as the desktop user so the command reaches the
# already-running Sublime instance and its plugin host.
if [[ "$(id -u)" -eq 0 ]]; then
  desktop_user="${LINUX_DESKTOP_USER:-$(loginctl list-sessions --no-legend | awk '$3 != "root" { print $3; exit }')}"
  if [[ -z "$desktop_user" ]]; then
    echo "could not find an active non-root desktop user" >&2
    exit 1
  fi
  desktop_uid="$(id -u "$desktop_user")"
  runtime_dir="/run/user/$desktop_uid"
  desktop_lang="$(awk -F= '$1 == "LANG" { gsub(/"/, "", $2); print $2; exit }' \
    /etc/locale.conf 2>/dev/null || true)"
  desktop_lang="${desktop_lang:-${LANG:-C.UTF-8}}"
  exec runuser -u "$desktop_user" -- env \
    HOME="$(getent passwd "$desktop_user" | cut -d: -f6)" \
    LANG="$desktop_lang" \
    DISPLAY="${DISPLAY:-:0}" \
    WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}" \
    XDG_RUNTIME_DIR="$runtime_dir" \
    DBUS_SESSION_BUS_ADDRESS="unix:path=$runtime_dir/bus" \
    bash "$0" "$@"
fi

sublime="${SUBLIME_TEXT:-$HOME/sublime_text/sublime_text}"
tests="${CAPTURE_TESTS:-$build_dir/capture-tests}"
out="${CAPTURE_OUTPUT:-$build_dir/sublime-metrics}"
limit="${METRICS_LIMIT:-0}"

if [[ ! -x "$sublime" ]]; then
  echo "Sublime Text not found at $sublime (override with SUBLIME_TEXT)" >&2
  exit 1
fi
if ! pgrep -f "$sublime" >/dev/null; then
  echo "Sublime Text is not running (launch $sublime, then rerun this script)" >&2
  exit 1
fi

read_config() { grep -vE '^[[:space:]]*(#|$)' "$1" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*$//'; }
faces=()
while IFS= read -r line; do faces+=("$line"); done < <(read_config "$tests/faces-linux.txt")
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
  stem="$(basename "$text_path" .txt)"
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      output_path="$out/$stem-$face-$size.json"
      command="metrics_probe {\"text_path\":\"$text_path\",\"face\":\"$face\",\"size\":$size,\"output_path\":\"$output_path\"}"
      "$sublime" --background --command "$command" </dev/null

      deadline=$((SECONDS + 20))
      while [[ ! -f "$output_path" && $SECONDS -lt $deadline ]]; do sleep 0.02; done
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
      if [[ "$limit" -gt 0 && "$current" -ge "$limit" ]]; then exit 0; fi
    done
  done
done
