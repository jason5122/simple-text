import os
import plistlib
import re
import subprocess
import sys


def parse_version(version_str):
    """'10.6' => [10, 6]"""
    return [int(s) for s in re.findall(r"(\d+)", version_str)]


def main():
    if len(sys.argv) != 2:
        raise Exception("usage: find_sdk.py <minimum SDK version>")
    min_sdk_version = sys.argv[1]

    job = subprocess.Popen(
        ["xcode-select", "-print-path"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    out, err = job.communicate()
    if job.returncode != 0:
        print(out, file=sys.stderr)
        print(err, file=sys.stderr)
        raise Exception("Error %d running xcrun" % job.returncode)

    dev_dir = out.decode("UTF-8").rstrip()
    sdk_dir = os.path.join(dev_dir, "SDKs")
    if not os.path.isdir(sdk_dir):
        raise Exception("Command Line Tools not found. Install via `xcode-select --install`")

    sdk_dir_list = os.listdir(sdk_dir)
    sdks = [re.findall(r"^MacOSX(\d+\.\d+)\.sdk$", s) for s in sdk_dir_list]
    sdks = [s[0] for s in sdks if s]  # [['10.5'], ['10.6']] => ['10.5', '10.6']
    sdks = [
        s
        for s in sdks  # ['10.5', '10.6'] => ['10.6']
        if parse_version(s) >= parse_version(min_sdk_version)
    ]
    if not sdks:
        raise Exception(f"No {min_sdk_version}+ SDK found. {sdk_dir} contains: {sdk_dir_list}")
    best_sdk = sorted(sdks, key=parse_version)[0]
    sdk_name = "MacOSX" + best_sdk + ".sdk"
    sdk_path = os.path.join(sdk_dir, sdk_name)

    print(sdk_path)


if __name__ == "__main__":
    if sys.platform != "darwin":
        raise Exception("This script only runs on Mac")
    sys.exit(main())
