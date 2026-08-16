#!/usr/bin/env python3
"""Download LLVM/clang toolchain packages from Chromium's prebuilt bucket.

Pins come from deps.json (llvm_toolchain). Pass the package name(s) to install; sync.py
computes the set from the host plus any selected targets/dev-tools.
"""

import argparse
import os
import sys

from download import extract_tar, get_os_cpu, load_pins

_PINS = load_pins("llvm_toolchain")
CLANG_REVISION = _PINS["clang_revision"]
CLANG_SUB_REVISION = _PINS["clang_sub_revision"]
RELEASE_VERSION = _PINS["release_version"]
PACKAGE_VERSION = f"{CLANG_REVISION}-{CLANG_SUB_REVISION}"
CDS_URL = "https://commondatastorage.googleapis.com/chromium-browser-clang"
# Host platforms with a prebuilt clang (sync derives its host-support guard from this).
PLATFORM_BY_OS_CPU = {
    ("linux", "x86_64"): "Linux_x64",
    ("mac", "x86_64"): "Mac",
    ("mac", "arm64"): "Mac_arm64",
    ("windows", "x86_64"): "Win",
}
OUTPUT_DIR = "third_party/llvm-toolchain"
PACKAGES = (
    "clang",
    "compiler-rt-mac",
    "compiler-rt-win",
    "compiler-rt-linux",
    "clangd",
    "clang-tidy",
    "llvm-code-coverage",
    "llvmobjdump",
    "libclang",
    "translation_unit",
)


def get_url(package_file, platform):
    return f"{CDS_URL}/{platform}/{package_file}-{PACKAGE_VERSION}.tar.xz"


def extract_mac_runtime(output_dir):
    extract_tar(get_url("clang-mac-runtime-library", "Mac"), output_dir)


def extract_windows_runtime(output_dir):
    extract_tar(get_url("clang-win-runtime-library", "Win"), output_dir)


def extract_linux_runtime(output_dir):
    path_prefixes = [
        f"lib/clang/{RELEASE_VERSION}/lib/x86_64-unknown-linux-gnu/",
        f"lib/clang/{RELEASE_VERSION}/lib/aarch64-unknown-linux-gnu/",
    ]
    extract_tar(get_url("clang", "Linux_x64"), output_dir, path_prefixes)


# compiler-rt-<os> installs that OS's runtime libraries (all arches it ships).
RUNTIME_EXTRACTORS = {
    "compiler-rt-mac": extract_mac_runtime,
    "compiler-rt-win": extract_windows_runtime,
    "compiler-rt-linux": extract_linux_runtime,
}


def get_platform():
    os_cpu = get_os_cpu()
    if os_cpu not in PLATFORM_BY_OS_CPU:
        sys.exit(f"unsupported platform {os_cpu[0]}/{os_cpu[1]}")
    return PLATFORM_BY_OS_CPU[os_cpu]


def download_packages(packages):
    # Navigate to project root (//build/deps -> //).
    os.chdir(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))

    print(f"Selected packages: {packages}")
    for package in packages:
        if extractor := RUNTIME_EXTRACTORS.get(package):
            extractor(OUTPUT_DIR)
        else:
            extract_tar(get_url(package, get_platform()), OUTPUT_DIR)


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog="available packages:\n" + "\n".join(f"  {package}" for package in PACKAGES),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("packages", nargs="+", metavar="PACKAGE", choices=PACKAGES)
    args = parser.parse_args()
    download_packages(args.packages)


if __name__ == "__main__":
    main()
