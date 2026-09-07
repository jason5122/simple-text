#!/bin/bash
# Captures the configured UI test cases through ui_conformance.
set -e

build_dir="$(cd "$(dirname "$0")" && pwd)"
out="$build_dir/ours-ui"
tests="$build_dir/capture-tests-ui"
crop="${1:-0,80,600,500}"

rm -rf "$out"
"$build_dir/ui_conformance" "$tests" "$out" --crop "$crop"
