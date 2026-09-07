#!/bin/bash
# Captures our Linux UI cases from ui_conformance's private backing framebuffer.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
tests="${LINUX_UI_CAPTURE_TESTS:-$build_dir/capture-tests-ui}"
out="${LINUX_UI_OUR_OUTPUT:-$build_dir/ours-ui}"
crop="${UI_CROP:-${1:-0,0,600,500}}"

# prlctl exec enters as root, but font selection and GTK must use the desktop user's session.
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
    LINUX_UI_CAPTURE_TESTS="$tests" \
    LINUX_UI_OUR_OUTPUT="$out" \
    UI_CROP="$crop" \
    bash "$0"
fi

if [[ ! -x "$build_dir/ui_conformance" ]]; then
  echo "ui_conformance not found at $build_dir" >&2
  exit 1
fi

rm -rf "$out"
"$build_dir/ui_conformance" "$tests" "$out" --crop "$crop"
