#!/bin/bash
# Captures the configured sidebar shaping probes using Linux-native window capture.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"

# Enter the active graphical session when this is launched through `prlctl exec` as root.
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
    LINUX_CAPTURE_BACKEND="${LINUX_CAPTURE_BACKEND:-auto}" \
    LINUX_CAPTURE_LOGICAL_TOP_INSET="${LINUX_CAPTURE_LOGICAL_TOP_INSET:-37}" \
    LINUX_UI_CAPTURE_TESTS="${LINUX_UI_CAPTURE_TESTS:-$build_dir/capture-tests-ui}" \
    LINUX_UI_SUBLIME_OUTPUT="${LINUX_UI_SUBLIME_OUTPUT:-$build_dir/sublime-ui}" \
    UI_CROP="${UI_CROP:-${1:-0,0,600,500}}" \
    bash "$0"
fi

server="$build_dir/capture_server"
sublime="${SUBLIME_TEXT:-$HOME/sublime_text/sublime_text}"
tests="${LINUX_UI_CAPTURE_TESTS:-$build_dir/capture-tests-ui}"
out="${LINUX_UI_SUBLIME_OUTPUT:-$build_dir/sublime-ui}"
crop="${UI_CROP:-${1:-0,0,600,500}}"
backend="${LINUX_CAPTURE_BACKEND:-auto}"
logical_top_inset="${LINUX_CAPTURE_LOGICAL_TOP_INSET:-37}"

if [[ ! -x "$server" ]]; then
  echo "capture_server not found at $server" >&2
  exit 1
fi
if [[ ! -x "$sublime" ]]; then
  echo "Sublime Text not found at $sublime (override with SUBLIME_TEXT)" >&2
  exit 1
fi

# Keep the command plugin beside Sublime's other User packages, for both portable and installed
# layouts. Avoid touching it when it is already current so a running instance need not reload it.
install_dir="$(dirname "$sublime")"
if [[ -d "$install_dir/Data/Packages" ]]; then
  user_packages="$install_dir/Data/Packages/User"
else
  user_packages="${XDG_CONFIG_HOME:-$HOME/.config}/sublime-text/Packages/User"
fi
mkdir -p "$user_packages"
if [[ ! -f "$user_packages/sidebar_render.py" ]] ||
   ! cmp -s "$build_dir/sidebar_render.py" "$user_packages/sidebar_render.py"; then
  cp "$build_dir/sidebar_render.py" "$user_packages/sidebar_render.py"
fi

wayland_capture=0
if [[ "$backend" != "x11" && -n "${WAYLAND_DISPLAY:-}" ]]; then
  wayland_capture=1
fi
if [[ "$wayland_capture" -eq 1 ]]; then
  pkill -f "$sublime" 2>/dev/null || true
  for _ in $(seq 40); do
    pgrep -f "$sublime" >/dev/null 2>&1 || break
    sleep 0.25
  done
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
run_sidebar_command() {
  local ready_path="$1"
  local ready_token="$2"
  local command="$3"
  local attempts=100
  while (( attempts-- > 0 )); do
    if (( attempts % 25 == 24 )); then
      "$sublime" --background --command "$command" </dev/null
    fi
    if [[ -f "$ready_path" && "$(<"$ready_path")" == "$ready_token" ]]; then
      return 0
    fi
    sleep 0.02
  done
  echo "timed out waiting for sidebar theme token $ready_token" >&2
  return 1
}
faces=()
while IFS= read -r line; do faces+=("$line"); done < <(read_config "$tests/faces-linux.txt")
sizes=()
while IFS= read -r line; do sizes+=("$line"); done < <(read_config "$tests/sizes.txt")
text_paths=("$tests"/texts/*.txt)
if [[ ${#faces[@]} -eq 0 || ${#sizes[@]} -eq 0 || ! -f "${text_paths[0]}" ]]; then
  echo "no UI test inputs found under $tests" >&2
  exit 2
fi
total=$((${#text_paths[@]} * ${#faces[@]} * ${#sizes[@]}))

rm -rf "$out"
mkdir -p "$out"
if [[ "$wayland_capture" -eq 1 ]]; then
  echo "Wayland capture: keep Sublime Text focused for the whole run" >&2
fi
"$sublime" --background --command 'focus_group {"group": 0}' </dev/null
sleep 0.25

fifo_dir="$(mktemp -d)"
request_fifo="$fifo_dir/request"
response_fifo="$fifo_dir/response"
baseline="$fifo_dir/baseline.png"
ready_path="$fifo_dir/sidebar-ready"
mkfifo "$request_fifo" "$response_fifo"
server_arguments=(--pid "$st_pid" --backend "$backend" --crop "$crop")
if [[ "$logical_top_inset" -ne 0 ]]; then
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

echo "$baseline" >&3
if ! read -r reply <&4 || [[ "$reply" == err* ]]; then
  echo "initial UI capture failed" >&2
  exit 1
fi

fails=0
current=0
server_gone=0
for text_path in "${text_paths[@]}"; do
  corpus="$(basename "$text_path" .txt)"
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      current=$((current + 1))
      ready_token="$current"
      rm -f "$ready_path"
      command="sidebar_render {\"text_path\":\"$text_path\",\"face\":\"$face\",\"size\":$size,\"ready_path\":\"$ready_path\",\"ready_token\":\"$ready_token\"}"
      run_sidebar_command "$ready_path" "$ready_token" "$command"
      printf '[%d/%d] ' "$current" "$total" >&2
      if ! echo "$out/$corpus-$face-$size.png" >&3 || ! read -r reply <&4; then
        server_gone=1
        break 3
      fi
      case "$reply" in err*) fails=$((fails + 1)) ;; esac
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
