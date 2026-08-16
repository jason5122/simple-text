#!/usr/bin/env python3
import os

from download import extract_zip, get_os_cpu, load_pins

CIPD_URL = "https://chrome-infra-packages.appspot.com"


def main():
    # Navigate to project root (//build/deps -> //).
    os.chdir(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))

    os_name, cpu = get_os_cpu()
    is_windows = os_name == "windows"
    exe_name = "gn.exe" if is_windows else "gn"

    rev = load_pins("gn")["cipd_rev"]
    url = f"{CIPD_URL}/dl/gn/gn/{os_name}-{cpu}/+/git_revision:{rev}"
    exe_path = extract_zip(url, "bin", exe_name)
    if not is_windows:
        os.chmod(exe_path, 0o755)


if __name__ == "__main__":
    main()
