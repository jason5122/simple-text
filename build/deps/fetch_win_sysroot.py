#!/usr/bin/env python3
"""Build the Windows sysroot (MSVC CRT + Windows SDK) by wrapping user-installed xwin.

xwin downloads and repacks Microsoft's CRT/SDK into a /winsysroot layout that clang-cl's
/winsysroot flag consumes. Install xwin yourself (`cargo install xwin` or
`brew install xwin`); this wraps its `splat` step with the flags our build needs.

xwin caches downloads but never skips `splat`, so we keep our own stamp and skip the
(slow) re-splat when the sysroot is already current. Config (archs + optional pinned
SDK/CRT versions) lives in deps.json under win_sysroot.
"""

import os
import shutil
import subprocess
import sys

from download import load_pins

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_DIR = "third_party/win-sysroot"
STAMP = os.path.join(OUTPUT_DIR, ".stamp")
# clang-cl's /winsysroot expects these directories under the output root.
REQUIRED_DIRS = ("VC", "Windows Kits")


def _xwin_version():
    result = subprocess.run(["xwin", "--version"], capture_output=True, text=True, check=True)
    return result.stdout.strip()


def _stamp_value(pins):
    # xwin isn't content-pinned, so key the stamp on what actually determines the output:
    # the xwin version, the arch set, and any pinned SDK/CRT versions. Any change -> re-splat.
    return (f"{_xwin_version()} archs={','.join(pins['archs'])} "
            f"sdk={pins.get('sdk_version')} crt={pins.get('crt_version')}")


def _have_sysroot():
    return all(os.path.isdir(os.path.join(OUTPUT_DIR, d)) for d in REQUIRED_DIRS)


def _read_stamp():
    if not os.path.exists(STAMP):
        return None
    with open(STAMP) as f:
        return f.read().strip()


def _write_stamp(value):
    with open(STAMP, "w") as f:
        f.write(value + "\n")


def main():
    if shutil.which("xwin") is None:
        sys.exit("xwin not found. Install it (`cargo install xwin` or `brew install xwin`), "
                 "then re-run.")

    # Navigate to project root (//build/deps -> //).
    os.chdir(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))

    pins = load_pins("win_sysroot")
    want = _stamp_value(pins)

    if _have_sysroot():
        current = _read_stamp()
        if current == want:
            print("win_sysroot: up to date")
            return
        if current is None:
            # Migration: a sysroot built before stamping exists; assume it's current
            # rather than forcing a slow re-splat.
            _write_stamp(want)
            print("win_sysroot: up to date (seeded stamp for existing sysroot)")
            return

    cmd = ["xwin", "--accept-license", "--temp", "--arch", ",".join(pins["archs"])]
    if pins.get("sdk_version"):
        cmd += ["--sdk-version", pins["sdk_version"]]
    if pins.get("crt_version"):
        cmd += ["--crt-version", pins["crt_version"]]
    cmd += ["splat", "--use-winsysroot-style", "--preserve-ms-arch-notation",
            "--include-debug-symbols", "--output", OUTPUT_DIR]

    print(f"Building win-sysroot: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

    missing = [d for d in REQUIRED_DIRS if not os.path.isdir(os.path.join(OUTPUT_DIR, d))]
    if missing:
        sys.exit(f"xwin splat did not produce {missing} under {OUTPUT_DIR}; aborting.")

    _write_stamp(want)
    print(f"win_sysroot: built ({want})")


if __name__ == "__main__":
    main()
