#!/usr/bin/env python3
import os

from download import extract_zip, get_os_cpu, load_pins

REPO_URL = "https://github.com/ninja-build/ninja"
ASSET_BY_OS_CPU = {
    ("mac", "x86_64"): "ninja-mac.zip",
    ("mac", "arm64"): "ninja-mac.zip",
    ("linux", "x86_64"): "ninja-linux.zip",
    ("linux", "arm64"): "ninja-linux-aarch64.zip",
    ("windows", "x86_64"): "ninja-win.zip",
    ("windows", "arm64"): "ninja-winarm64.zip",
}


def main():
    # Navigate to project root (//build/deps -> //).
    os.chdir(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))

    os_name, cpu = get_os_cpu()
    is_windows = os_name == "windows"
    exe_name = "ninja.exe" if is_windows else "ninja"

    version = load_pins("ninja")["version"]
    url = f"{REPO_URL}/releases/download/{version}/{ASSET_BY_OS_CPU[(os_name, cpu)]}"
    exe_path = extract_zip(url, "bin", exe_name)
    if not is_windows:
        os.chmod(exe_path, 0o755)


if __name__ == "__main__":
    main()
