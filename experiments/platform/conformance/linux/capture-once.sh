#!/bin/bash
# Captures one settled frame from a Linux window through the selected native backend.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
output="${LINUX_CAPTURE_OUTPUT:?set LINUX_CAPTURE_OUTPUT}"
backend="${LINUX_CAPTURE_BACKEND:-auto}"
crop="${LINUX_CAPTURE_CROP:-0,0,0,0}"
pid="${LINUX_CAPTURE_PID:-0}"
owner="${LINUX_CAPTURE_OWNER:-capture-target}"

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
    LINUX_CAPTURE_BACKEND="$backend" \
    LINUX_CAPTURE_CROP="$crop" \
    LINUX_CAPTURE_OUTPUT="$output" \
    LINUX_CAPTURE_OWNER="$owner" \
    LINUX_CAPTURE_PID="$pid" \
    bash "$0"
fi

arguments=(--backend "$backend" --crop "$crop")
if [[ "$pid" -gt 0 ]]; then
  arguments+=(--pid "$pid")
else
  arguments+=(--owner "$owner")
fi

mkdir -p "$(dirname "$output")"
printf '%s\nquit\n' "$output" | "$build_dir/capture_server" "${arguments[@]}"
