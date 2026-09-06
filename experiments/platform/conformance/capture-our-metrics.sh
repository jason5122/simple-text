#!/bin/bash
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
tests="$build_dir/capture-tests"
out="$build_dir/our-metrics"
limit="${METRICS_LIMIT:-0}"

# prlctl enters a Linux guest as root. Fontconfig has per-user configuration and caches, so run
# the probe as the same desktop user as Sublime or fallback selection can differ even when all
# primary-font metrics agree.
if [[ "$(uname -s)" == "Linux" && "$(id -u)" -eq 0 ]]; then
  desktop_user="${LINUX_DESKTOP_USER:-$(loginctl list-sessions --no-legend | awk '$3 != "root" { print $3; exit }')}"
  if [[ -z "$desktop_user" ]]; then
    echo "could not find an active non-root desktop user" >&2
    exit 1
  fi
  desktop_lang="$(awk -F= '$1 == "LANG" { gsub(/"/, "", $2); print $2; exit }' \
    /etc/locale.conf 2>/dev/null || true)"
  desktop_lang="${desktop_lang:-${LANG:-C.UTF-8}}"
  exec runuser -u "$desktop_user" -- env \
    HOME="$(getent passwd "$desktop_user" | cut -d: -f6)" \
    LANG="$desktop_lang" \
    METRICS_LIMIT="$limit" \
    bash "$0"
fi

if [[ -f "$build_dir/metrics_conformance.exe" ]]; then
  vm="${PARALLELS_VM:-Windows 11}"
  windows_share="${WINDOWS_SHARE:-\\\\Mac\\win-arm64}"
  if ! command -v prlctl >/dev/null; then
    echo "prlctl not found; run this script from the macOS host" >&2
    exit 1
  fi
  prlctl exec "$vm" --current-user \
    powershell.exe -NoProfile -ExecutionPolicy Bypass \
    -File "$windows_share\\capture-our-metrics.ps1" \
    -TestsDirectory "$windows_share\\capture-tests" \
    -OutputDirectory "$windows_share\\our-metrics" \
    -ExecutablePath "$windows_share\\metrics_conformance.exe" \
    -Limit "$limit"
  exit
fi

read_config() {
  grep -vE '^[[:space:]]*(#|$)' "$1" | sed -E 's/^[[:space:]]*//; s/[[:space:]]*$//'
}

face_files=("$tests"/faces-*.txt)
text_paths=("$tests"/texts/*.txt)
if [[ ${#face_files[@]} -ne 1 || ! -f "${face_files[0]}" || ! -f "${text_paths[0]}" ]]; then
  echo "expected one faces file and at least one text corpus under $tests" >&2
  exit 2
fi

faces=()
while IFS= read -r line; do faces+=("$line"); done < <(read_config "${face_files[0]}")
sizes=()
while IFS= read -r line; do sizes+=("$line"); done < <(read_config "$tests/sizes.txt")
if [[ ${#faces[@]} -eq 0 || ${#sizes[@]} -eq 0 ]]; then
  echo "no faces or sizes under $tests" >&2
  exit 2
fi

rm -rf "$out"
mkdir -p "$out"
total=$((${#text_paths[@]} * ${#faces[@]} * ${#sizes[@]}))
current=0

for text_path in "${text_paths[@]}"; do
  text_name=$(basename "$text_path")
  stem=${text_name%.txt}
  for face in "${faces[@]}"; do
    for size in "${sizes[@]}"; do
      output_name="$stem-$face-$size.json"
      output_path="$out/$output_name"
      "$build_dir/metrics_conformance" "$text_path" "$face" "$size" "$output_path"

      current=$((current + 1))
      printf '[%d/%d] %s\n' "$current" "$total" "$output_path"
      if [[ "$limit" -gt 0 && "$current" -ge "$limit" ]]; then
        exit 0
      fi
    done
  done
done
