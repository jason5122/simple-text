import os
import platform
import sys
import tarfile
import tempfile
import time
import urllib.error
import urllib.request
import zipfile


def get_os_cpu():
    system = platform.system().lower()  # darwin, linux, windows
    machine = platform.machine().lower()  # x86_64, amd64, arm64, aarch64
    os_name = "mac" if system == "darwin" else system
    if machine in ("x86_64", "amd64"):
        cpu = "x86_64"
    elif machine in ("arm64", "aarch64"):
        cpu = "arm64"
    else:
        sys.exit(f"unsupported architecture: {machine}")
    return os_name, cpu


def extract_zip(url, output_dir, member):
    with tempfile.TemporaryFile() as f:
        download(url, f)
        f.seek(0)
        with zipfile.ZipFile(f) as z:
            z.extract(member, output_dir)
    return os.path.join(output_dir, member)


def extract_tar(url, output_dir, path_prefixes=None):
    with tempfile.TemporaryFile() as f:
        download(url, f)
        f.seek(0)
        with tarfile.open(mode="r:*", fileobj=f) as t:
            members = t.getmembers()
            if path_prefixes is not None:
                members = [m for m in members if any(m.name.startswith(p) for p in path_prefixes)]
            t.extractall(path=output_dir, members=members)


def download(url, output_file, num_retries=3):
    retry_wait_s = 5
    while True:
        try:
            print(f"Downloading {url}")
            with urllib.request.urlopen(url, timeout=30) as response:
                bytes_done = 0
                while True:
                    chunk = response.read(4096)
                    if not chunk:
                        break
                    output_file.write(chunk)
                    bytes_done += len(chunk)
            print(f"Downloaded {human_bytes(bytes_done)}.")
            return
        except (ConnectionError, urllib.error.URLError) as e:
            print(e)
            if num_retries == 0 or (isinstance(e, urllib.error.HTTPError) and e.code == 404):
                raise
            num_retries -= 1
            output_file.seek(0)
            output_file.truncate()
            print(f"Retrying in {retry_wait_s} s ...")
            sys.stdout.flush()
            time.sleep(retry_wait_s)
            retry_wait_s *= 2


def human_bytes(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024 or unit == "GB":
            return f"{n} B" if unit == "B" else f"{n:.1f} {unit}"
        n /= 1024
