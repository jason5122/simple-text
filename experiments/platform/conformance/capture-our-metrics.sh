#!/bin/bash
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
tests="$build_dir/capture-tests"
out="$build_dir/our-metrics"
limit="${METRICS_LIMIT:-0}"

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
