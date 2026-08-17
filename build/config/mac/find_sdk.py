"""Print the path to a macOS SDK for the build.

find_sdk.py 14              # discover the host's SDK via xcrun (mac host)
find_sdk.py --sysroot DIR   # find a MacOSX*.sdk staged under DIR (any host)
"""

import glob
import os
import subprocess
import sys


def parse_version(version_str):
    """'14.0' => [14, 0]"""
    return [int(part) for part in version_str.split(".")]


def _from_xcrun(min_version):
    def xcrun(*args):
        try:
            return subprocess.check_output(["xcrun", "--sdk", "macosx", *args], text=True).strip()
        except (OSError, subprocess.CalledProcessError):
            sys.exit(
                "Could not query the macOS SDK. Install Xcode or the Command Line Tools "
                "(`xcode-select --install`) and select it (`xcode-select -s` or DEVELOPER_DIR)."
            )

    sdk_version = xcrun("--show-sdk-version")
    if parse_version(sdk_version) < parse_version(min_version):
        sys.exit(
            f"macOS SDK {sdk_version} is older than the required {min_version}; "
            f"install a newer Xcode or Command Line Tools."
        )
    return xcrun("--show-sdk-path")


def _from_sysroot(sysroot):
    if sysroot.rstrip("/").endswith(".sdk"):
        return sysroot
    xcode_sdks = "Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs"
    for base in (sysroot, os.path.join(sysroot, xcode_sdks)):
        if matches := sorted(glob.glob(os.path.join(base, "MacOSX*.sdk"))):
            return matches[-1]  # highest-versioned
    sys.exit(
        f"no MacOSX*.sdk staged under {sysroot}; put a macOS SDK there "
        f"(extract it from Xcode -- see the docs)."
    )


def main(argv):
    if len(argv) == 2 and not argv[1].startswith("--"):
        print(_from_xcrun(argv[1]))
    elif len(argv) == 3 and argv[1] == "--sysroot":
        print(_from_sysroot(argv[2]))
    else:
        raise Exception("usage: find_sdk.py <min-version> | --sysroot DIR")


if __name__ == "__main__":
    main(sys.argv)
