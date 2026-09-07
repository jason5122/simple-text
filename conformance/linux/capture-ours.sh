#!/bin/bash
# Captures our Linux renderer from its private RGB8 backing framebuffer.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
tests="${LINUX_CAPTURE_TESTS:-$build_dir/capture-tests}"
out="${LINUX_OUR_OUTPUT:-$build_dir/ours}"
crop="${BUFFER_CROP:-0,0,1600,600}"
limit="${BUFFER_LIMIT:-0}"
filter="${BUFFER_FILTER:-}"
if [[ -n "${BUFFER_FILTER_B64:-}" ]]; then
  filter="$(printf %s "$BUFFER_FILTER_B64" | base64 --decode)"
fi
hold_seconds="${BUFFER_HOLD_SECONDS:-0}"

# prlctl exec enters as root, but fontconfig selection must use the same user configuration and
# caches as Sublime Text. Re-enter the active desktop session; GTK also needs its display bus.
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
  # PX_USE_GL is opt-in: only forward it when actually set, since `env KEY=` with an
  # empty value still defines KEY in the child environment (unlike leaving it unset),
  # which would force the GL path on even for a default/software-path run.
  env_args=(
    HOME="$(getent passwd "$desktop_user" | cut -d: -f6)"
    LANG="$desktop_lang"
    DISPLAY="${DISPLAY:-:0}"
    WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
    XDG_RUNTIME_DIR="$runtime_dir"
    DBUS_SESSION_BUS_ADDRESS="unix:path=$runtime_dir/bus"
    XAUTHORITY="$xauthority"
    LINUX_CAPTURE_TESTS="$tests"
    LINUX_OUR_OUTPUT="$out"
    BUFFER_CROP="$crop"
    BUFFER_FILTER="$filter"
    BUFFER_HOLD_SECONDS="$hold_seconds"
    BUFFER_LIMIT="$limit"
  )
  if [[ -n "${PX_USE_GL:-}" ]]; then
    env_args+=(PX_USE_GL="$PX_USE_GL")
  fi
  exec runuser -u "$desktop_user" -- env "${env_args[@]}" bash "$0"
fi

if [[ ! -x "$build_dir/buffer_conformance" ]]; then
  echo "buffer_conformance not found at $build_dir" >&2
  exit 1
fi

rm -rf "$out"
mkdir -p "$out"
BUFFER_FILTER="$filter" BUFFER_HOLD_SECONDS="$hold_seconds" BUFFER_LIMIT="$limit" \
  "$build_dir/buffer_conformance" "$tests" "$out" --crop "$crop"
