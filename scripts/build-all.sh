#!/bin/bash
set -e

cd "$(dirname "$0")/.."

export NINJA_STATUS="[%f/%t %w] "

# Overridable via env: MODE=release|debug  SANITIZERS=1|0
mode="${MODE:-debug}"
use_sanitizers="${SANITIZERS:-1}"
case "$mode" in
release) is_release=true ;;
debug) is_release=false ;;
*)
    echo "MODE must be 'release' or 'debug', got '$mode'" >&2
    exit 1
    ;;
esac

targets=(
    mac-arm64
    mac-x64
    linux-arm64
    linux-x64
    win-arm64
    win-x64
)

for t in "${targets[@]}"; do
    os="${t%-*}"
    cpu="${t#*-}"
    sanitizers=""
    # ASan/UBSan aren't supported on Windows ARM64.
    if [[ "$use_sanitizers" == "1" && "$t" != "win-arm64" ]]; then
        sanitizers="is_asan=true is_ubsan=true enable_fuzztest_fuzz=true"
    fi
    mac_sdk_path='"//third_party/mac-sysroot/MacOSX14.0.sdk"'

    out="out/$t-$mode"
    rm -rf "$out"
    bin/gn gen "$out" --args="target_os=\"$os\" target_cpu=\"$cpu\" is_release=$is_release $sanitizers mac_sdk_path=$mac_sdk_path"
    bin/ninja -C "$out"
done
