import os
import sys
import subprocess


# TODO: Handle errors and make script more robust.
def main():
    p = subprocess.run(
        ["xcrun", "--sdk", "macosx", "--show-sdk-path"],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    sdk_dir = p.stdout.strip()
    sys.stdout.write(sdk_dir)


if __name__ == "__main__":
    if sys.platform != "darwin":
        raise Exception("This script only runs on Mac")
    sys.exit(main())
