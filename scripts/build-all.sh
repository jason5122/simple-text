#!/bin/bash
set -e

cd "$(dirname "$0")/.."

targets=(
    mac-arm64
    # mac-x64
    linux-arm64
    linux-x64
    win-arm64
    win-x64
)

for t in "${targets[@]}"; do
    os="${t%-*}"
    cpu="${t#*-}"
    # ASan/UBSan aren't supported on Windows ARM64.
    sanitizers="is_asan=true is_ubsan=true"
    if [[ "$t" == "win-arm64" ]]; then
        sanitizers=""
    fi
    rm -rf "out/$t"
    bin/gn gen "out/$t" --args="target_os=\"$os\" target_cpu=\"$cpu\" $sanitizers"
    bin/ninja -C "out/$t"
done
