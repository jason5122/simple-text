#!/usr/bin/env python3
"""Download Debian sysroots for cross-compiling to Linux.

A sysroot lets us build against the oldest Debian we support regardless of the host.
The catalog (sha256 + extract dir per name) lives in deps.json under
linux_sysroot; pass the sysroot name(s) to install.
"""

import argparse
import os

from download import extract_tar, load_pins

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SYSROOT_URL = "https://commondatastorage.googleapis.com/chrome-linux-sysroot"
OUTPUT_DIR = "third_party/linux-sysroot"

# Each sysroot is hosted content-addressed by its sha256, so the sha256 IS the object
# name in the URL (not the tarball filename). name -> {"sha256", "dir"}.
SYSROOTS = load_pins("linux_sysroot")


def download_sysroots(names):
    # Navigate to project root (//build/deps -> //).
    os.chdir(os.path.join(SCRIPT_DIR, os.pardir, os.pardir))

    for name in names:
        entry = SYSROOTS[name]
        extract_tar(f"{SYSROOT_URL}/{entry['sha256']}", os.path.join(OUTPUT_DIR, entry["dir"]))


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog="available sysroots:\n" + "\n".join(f"  {name}" for name in SYSROOTS),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "sysroots",
        help="Sysroots to download.",
        nargs="+",
        metavar="SYSROOT",
        choices=list(SYSROOTS),
    )
    args = parser.parse_args()
    download_sysroots(args.sysroots)


if __name__ == "__main__":
    main()
