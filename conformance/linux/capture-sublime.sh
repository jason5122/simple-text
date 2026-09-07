#!/bin/bash
# Captures the Sublime Text reference suite using Linux-native window capture.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"

# prlctl exec enters a Linux guest as root unless guest authentication is configured. Re-enter as
# the active desktop user with the environment needed to reach that user's Wayland/X11 session.
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
  xauthority=""
  for candidate in "$runtime_dir"/.mutter-Xwaylandauth.*; do
    if [[ -f "$candidate" ]]; then
      xauthority="$candidate"
      break
    fi
  done
  exec runuser -u "$desktop_user" -- env \
    HOME="$(getent passwd "$desktop_user" | cut -d: -f6)" \
    LANG="$desktop_lang" \
    DISPLAY="${DISPLAY:-:0}" \
    WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}" \
    XDG_RUNTIME_DIR="$runtime_dir" \
    DBUS_SESSION_BUS_ADDRESS="unix:path=$runtime_dir/bus" \
    XAUTHORITY="$xauthority" \
    bash "$0" "$@"
fi

server="$build_dir/capture_server"
sublime="${SUBLIME_TEXT:-$HOME/sublime_text/sublime_text}"
tests="${CAPTURE_TESTS:-$build_dir/capture-tests}"
out="${CAPTURE_OUTPUT:-$build_dir/sublime}"
crop="${1:-0,0,1600,600}"
limit="${CAPTURE_LIMIT:-0}"
filter="${CAPTURE_FILTER:-}"
if [[ -n "${CAPTURE_FILTER_B64:-}" ]]; then
  filter="$(printf %s "$CAPTURE_FILTER_B64" | base64 --decode)"
fi
backend="${LINUX_CAPTURE_BACKEND:-auto}"
# Height of Sublime's client-side title bar, which Mutter includes in a window capture. 37 is the
# bare title bar of a maximized window, which is what the relaunch below produces. A floating window
# adds its drop shadow on top and needs roughly ten more, so this has to be recalibrated if the
# window ever stops coming back maximized -- getting it wrong shifts every capture and fails the
# whole diff rather than any one shot.
logical_top_inset="${LINUX_CAPTURE_LOGICAL_TOP_INSET:-37}"

if [[ ! -x "$server" ]]; then
  echo "capture_server not found at $server" >&2
  exit 1
fi
if [[ ! -x "$sublime" ]]; then
  echo "Sublime Text not found at $sublime (override with SUBLIME_TEXT)" >&2
  exit 1
fi

wayland_capture=0
if [[ "$backend" != "x11" && -n "${WAYLAND_DISPLAY:-}" ]]; then
  wayland_capture=1
fi

# Mutter's RecordWindow attaches to whatever window holds focus, and a Wayland client cannot raise
# itself without an activation token, so the only dependable way to put Sublime there is to map a
# fresh window: newly mapped windows take focus. Leaving an existing instance in place captures
# whatever happened to be focused instead, which yields one blank frame and then nothing.
if [[ "$wayland_capture" -eq 1 ]]; then
  pkill -f "$sublime" 2>/dev/null || true
  # A replacement launched before the old process releases its single-instance lock just exits, so
  # wait for the whole group -- editor, plugin hosts, crash handler -- to go away first.
  for _ in $(seq 40); do
    pgrep -f "$sublime" >/dev/null 2>&1 || break
    sleep 0.25
  done
  # Sublime re-execs itself detached; setsid keeps that survivor out of this script's process group.
  setsid "$sublime" </dev/null >/dev/null 2>&1 &
  for _ in $(seq 60); do
    if pgrep -f "$(dirname "$sublime")/plugin_host" >/dev/null 2>&1; then
      break
    fi
    sleep 0.25
  done
  sleep 0.5
fi

st_pid="$(pgrep -f "$sublime" | sort -n | head -1)"
if [[ -z "$st_pid" ]]; then
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
total=$((${#text_paths[@]} * ${#faces[@]} * ${#sizes[@]}))

rm -rf "$out"
mkdir -p "$out"

# The stream stays attached to the window focused at this point, so nothing may steal focus for the
# rest of the run. On X11, capture_server locates the window by PID instead and focus is irrelevant.
if [[ "$wayland_capture" -eq 1 ]]; then
  echo "Wayland capture: keep Sublime Text focused for the whole run" >&2
fi
"$sublime" --background --command 'focus_group {"group": 0}' </dev/null
sleep 0.25

fifo_dir="$(mktemp -d)"
request_fifo="$fifo_dir/request"
response_fifo="$fifo_dir/response"
mkfifo "$request_fifo" "$response_fifo"
server_arguments=(--pid "$st_pid" --backend "$backend" --crop "$crop")
if [[ $# -eq 0 && "$logical_top_inset" -ne 0 ]]; then
  server_arguments+=(--logical-top-inset "$logical_top_inset")
fi
"$server" "${server_arguments[@]}" <"$request_fifo" >"$response_fifo" &
server_pid=$!
exec 3>"$request_fifo"
exec 4<"$response_fifo"

cleanup() {
  exec 3>&- || true
  if kill -0 "$server_pid" 2>/dev/null; then kill "$server_pid" 2>/dev/null || true; fi
  wait "$server_pid" 2>/dev/null || true
  rm -rf "$fifo_dir"
}
trap cleanup EXIT

fails=0
current=0
server_gone=0
for text_path in "${text_paths[@]}"; do
  stem="$(basename "$text_path" .txt)"
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      output_name="$stem-$face-$size.png"
      if [[ -n "$filter" && "$output_name" != *"$filter"* ]]; then
        continue
      fi
      "$sublime" --background --command \
        "rasterizer_render {\"text_path\": \"$text_path\", \"face\": \"$face\", \"size\": $size}" \
        </dev/null
      current=$((current + 1))
      printf '[%d/%d] ' "$current" "$total" >&2
      # capture_server exits on its own once enough consecutive captures time out, which closes
      # both fifos. Stop here rather than spending the rest of the run writing into a dead pipe.
      if ! echo "$out/$output_name" >&3 || ! read -r reply <&4; then
        server_gone=1
        break 3
      fi
      case "$reply" in err*) fails=$((fails + 1)) ;; esac
      if [[ "$limit" -gt 0 && "$current" -ge "$limit" ]]; then
        break 3
      fi
    done
  done
done

if [[ "$server_gone" -eq 0 ]]; then
  echo quit >&3
fi
exec 3>&-
wait "$server_pid" || true
trap - EXIT
rm -rf "$fifo_dir"
if [[ "$server_gone" -eq 1 ]]; then
  echo "capture server stopped early, abandoning the run" >&2
  exit 1
fi
if [[ "$fails" -ne 0 ]]; then
  echo "$fails capture(s) failed" >&2
  exit 1
fi
