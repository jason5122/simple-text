"""Sync pinned build dependencies.

python3 build/sync_deps.py            # sync defaults
python3 build/sync_deps.py ninja gn   # just the named component(s)
python3 build/sync_deps.py --dry-run  # print the plan, fetch nothing
"""

import argparse
import os
import platform as _platform  # aliased to avoid clashing with the Platform class
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.error
import urllib.request
import zipfile
from dataclasses import dataclass

_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # //build/sync_deps.py -> //
BIN_DIR = os.path.join(_ROOT, "bin")
THIRD_PARTY_DIR = os.path.join(_ROOT, "third_party")
TOOLCHAIN_DIR = os.path.join(THIRD_PARTY_DIR, "llvm-toolchain")


# Low-level fetch: download a URL to a temp file, then extract the archive into place.


def _human_bytes(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024 or unit == "GB":
            return f"{n} B" if unit == "B" else f"{n:.1f} {unit}"
        n /= 1024


def _download(url, output_file, num_retries=3):
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
            print(f"Downloaded {_human_bytes(bytes_done)}.")
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


def _extract_zip(url, output_dir, member):
    with tempfile.TemporaryFile() as f:
        _download(url, f)
        f.seek(0)
        with zipfile.ZipFile(f) as z:
            z.extract(member, output_dir)
    return os.path.join(output_dir, member)


def _extract_tar(url, output_dir, path_prefixes=None):
    with tempfile.TemporaryFile() as f:
        _download(url, f)
        f.seek(0)
        with tarfile.open(mode="r:*", fileobj=f) as t:
            members = t.getmembers()
            if path_prefixes is not None:
                members = [m for m in members if any(m.name.startswith(p) for p in path_prefixes)]
            t.extractall(path=output_dir, members=members)


@dataclass(frozen=True)
class Platform:
    os_name: str  # mac | linux | windows
    cpu: str  # x86_64 | arm64

    @classmethod
    def host(cls):
        system = _platform.system().lower()  # darwin, linux, windows
        machine = _platform.machine().lower()  # x86_64, amd64, arm64, aarch64
        os_name = "mac" if system == "darwin" else system
        if machine in ("x86_64", "amd64"):
            cpu = "x86_64"
        elif machine in ("arm64", "aarch64"):
            cpu = "arm64"
        else:
            sys.exit(f"unsupported architecture: {machine}")
        return cls(os_name, cpu)

    @property
    def is_windows(self):
        return self.os_name == "windows"

    @property
    def exe_suffix(self):
        return ".exe" if self.is_windows else ""


@dataclass(frozen=True)
class Status:
    fresh: bool
    reason: str = ""  # why a fetch is needed; shown in --dry-run

    @staticmethod
    def up_to_date():
        return Status(True)

    @staticmethod
    def needs_fetch(reason):
        return Status(False, reason)


class Component:
    """A pinned build dependency. Present when its install path exists."""

    name = ""
    target_os: str | None = None  # OS this supports; None = host-agnostic (any build needs it)
    optional = False  # dev-only extra, excluded from the default set

    def install_path(self, platform):
        raise NotImplementedError

    def status(self, platform):
        if os.path.exists(self.install_path(platform)):
            return Status.up_to_date()
        return Status.needs_fetch("missing")

    def fetch(self, platform):
        raise NotImplementedError


class BinaryComponent(Component):
    """A single executable extracted from a downloaded zip into //bin."""

    def install_path(self, platform):
        return os.path.join(BIN_DIR, self.name + platform.exe_suffix)

    def _url(self, platform):
        raise NotImplementedError

    def fetch(self, platform):
        os.makedirs(BIN_DIR, exist_ok=True)
        exe = _extract_zip(self._url(platform), BIN_DIR, self.name + platform.exe_suffix)
        if not platform.is_windows:
            os.chmod(exe, 0o755)


class Gn(BinaryComponent):
    name = "gn"
    revision = "64cfb8344ec3e8585a89a3836716a026e2771fcb"
    _CIPD_URL = "https://chrome-infra-packages.appspot.com"

    def _url(self, platform):
        return f"{self._CIPD_URL}/dl/gn/gn/{platform.os_name}-{platform.cpu}/+/git_revision:{self.revision}"


class Ninja(BinaryComponent):
    name = "ninja"
    version = "v1.13.2"
    _REPO_URL = "https://github.com/ninja-build/ninja"
    # Ninja's release assets aren't named uniformly across platforms.
    _ASSETS = {
        ("mac", "x86_64"): "ninja-mac.zip",
        ("mac", "arm64"): "ninja-mac.zip",
        ("linux", "x86_64"): "ninja-linux.zip",
        ("linux", "arm64"): "ninja-linux-aarch64.zip",
        ("windows", "x86_64"): "ninja-win.zip",
        ("windows", "arm64"): "ninja-winarm64.zip",
    }

    def _url(self, platform):
        asset = self._ASSETS[(platform.os_name, platform.cpu)]
        return f"{self._REPO_URL}/releases/download/{self.version}/{asset}"


class ToolchainPackage(Component):
    clang_revision = "llvmorg-23-init-19482-g53d18800"
    clang_sub_revision = 1
    release_version = "23"

    _CDS_URL = "https://commondatastorage.googleapis.com/chromium-browser-clang"
    # Host platform -> the Chromium bucket that ships that host's prebuilt clang.
    _HOST_BUCKETS = {
        ("linux", "x86_64"): "Linux_x64",
        ("mac", "x86_64"): "Mac",
        ("mac", "arm64"): "Mac_arm64",
        ("windows", "x86_64"): "Win",
    }

    # Package tarball basename on the CDS; each subclass sets this.
    package_file: str

    @property
    def version(self):
        return f"{self.clang_revision}-{self.clang_sub_revision}"

    def _bucket(self, platform):
        raise NotImplementedError

    def _stamp_path(self):
        return os.path.join(TOOLCHAIN_DIR, ".stamps", self.name)

    def _url(self, platform):
        return f"{self._CDS_URL}/{self._bucket(platform)}/{self.package_file}-{self.version}.tar.xz"

    def status(self, platform):
        stamp = self._stamp_path()
        if not os.path.exists(stamp):
            return Status.needs_fetch("missing")
        with open(stamp) as f:
            installed = f.read().strip()
        return Status.up_to_date() if installed == self.version else Status.needs_fetch("stale")

    def fetch(self, platform):
        os.makedirs(TOOLCHAIN_DIR, exist_ok=True)
        self._extract(platform)
        self._write_stamp()

    def _extract(self, platform):
        # Whole tarball by default; subclasses override to keep only part of it.
        _extract_tar(self._url(platform), TOOLCHAIN_DIR)

    def _write_stamp(self):
        os.makedirs(os.path.dirname(self._stamp_path()), exist_ok=True)
        with open(self._stamp_path(), "w") as f:
            f.write(self.version + "\n")


class ToolchainTool(ToolchainPackage):
    """A tool tarball named after the package, taken from the host's own bucket."""

    def __init__(self, name, optional=False):
        self.name = name
        self.package_file = name
        self.optional = optional

    def _bucket(self, platform):
        key = (platform.os_name, platform.cpu)
        if key not in self._HOST_BUCKETS:
            sys.exit(f"no prebuilt clang for host {platform.os_name}-{platform.cpu}")
        return self._HOST_BUCKETS[key]


class RuntimeLibrary(ToolchainPackage):
    """A compiler-rt runtime for one target OS, from that OS's fixed bucket.

    These ignore the host bucket: building the Windows runtime on a Mac host still pulls
    the Win package.
    """

    def __init__(self, name, package_file, bucket, target_os):
        self.name = name
        self.package_file = package_file
        self._bucket_name = bucket
        self.target_os = target_os

    def _bucket(self, platform):
        return self._bucket_name


class LinuxRuntimeLibrary(RuntimeLibrary):
    """compiler-rt for Linux, which has no standalone runtime package: pull the full
    Linux clang tarball and keep only the compiler-rt lib directories."""

    def __init__(self):
        super().__init__("compiler-rt-linux", "clang", "Linux_x64", target_os="linux")

    def _extract(self, platform):
        keep = [
            f"lib/clang/{self.release_version}/lib/{triple}/"
            for triple in ("x86_64-unknown-linux-gnu", "aarch64-unknown-linux-gnu")
        ]
        _extract_tar(self._url(platform), TOOLCHAIN_DIR, keep)


class LibcxxSource(Component):
    name = "libcxx"
    llvm_commit = "53d18800eda3b7407e53366f27ca78e922c6e0db"

    _LLVM_REPO = "https://github.com/llvm/llvm-project.git"
    # Upstream subtree -> destination under third_party/. Whole subtrees, on purpose.
    _SUBTREES = {
        "libcxx/include": "libc++/include",
        "libcxx/src": "libc++/src",
        "libcxxabi/include": "libc++abi/include",
        "libcxxabi/src": "libc++abi/src",
        "libc/shared": "llvm-libc/shared",
        "libc/src/__support": "llvm-libc/src/__support",
        "libc/hdr": "llvm-libc/hdr",
        "libc/include": "llvm-libc/include",
    }
    # Top-level vendored dirs; each gets the upstream LICENSE and a stamp.
    _VENDOR_DIRS = ("libc++", "libc++abi", "llvm-libc")

    def _stamp_path(self, vendor_dir):
        return os.path.join(THIRD_PARTY_DIR, vendor_dir, ".stamp")

    def status(self, platform):
        # A stamp per vendored dir, so deleting any of them re-triggers a fetch.
        for vendor_dir in self._VENDOR_DIRS:
            stamp = self._stamp_path(vendor_dir)
            if not os.path.exists(stamp):
                return Status.needs_fetch("missing")
            with open(stamp) as f:
                if f.read().strip() != self.llvm_commit:
                    return Status.needs_fetch("stale")
        return Status.up_to_date()

    def fetch(self, platform):
        self._check_toolchain_coherence()
        if shutil.which("git") is None:
            sys.exit("git is required to fetch libc++ sources.")
        with tempfile.TemporaryDirectory() as work_dir:
            self._sparse_fetch(work_dir)
            self._vendor(work_dir)

    def _check_toolchain_coherence(self):
        # The prebuilt toolchain's version ends in -g<short-sha> of this commit; refuse to
        # vendor sources that don't match the compiler they'll be built with.
        short_sha = ToolchainPackage.clang_revision.rsplit("-g", 1)[-1]
        if not self.llvm_commit.startswith(short_sha):
            sys.exit(
                f"libcxx commit {self.llvm_commit} does not match toolchain "
                f"{ToolchainPackage.clang_revision} (-g{short_sha}); roll both together."
            )

    def _git(self, work_dir, *args):
        subprocess.run(["git", *args], cwd=work_dir, check=True)

    def _sparse_fetch(self, work_dir):
        self._git(work_dir, "init", "-q")
        self._git(work_dir, "remote", "add", "origin", self._LLVM_REPO)
        # --filter=blob:none fetches trees but no file contents; the sparse set then limits
        # which blobs the checkout actually pulls.
        self._git(
            work_dir,
            "fetch",
            "-q",
            "--depth",
            "1",
            "--filter=blob:none",
            "origin",
            self.llvm_commit,
        )
        self._git(work_dir, "sparse-checkout", "set", "--cone", *sorted(self._SUBTREES))
        self._git(work_dir, "checkout", "-q", "FETCH_HEAD")

    def _vendor(self, work_dir):
        for upstream, dest in self._SUBTREES.items():
            dst = os.path.join(THIRD_PARTY_DIR, dest)
            # Replace only the vendored subtree, leaving hand-written siblings (BUILD.gn,
            # __config_site, ...) in place.
            shutil.rmtree(dst, ignore_errors=True)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copytree(os.path.join(work_dir, upstream), dst)
        for vendor_dir in self._VENDOR_DIRS:
            shutil.copy(
                os.path.join(work_dir, "LICENSE.TXT"),
                os.path.join(THIRD_PARTY_DIR, vendor_dir, "LICENSE.TXT"),
            )
            self._write_stamp(vendor_dir)

    def _write_stamp(self, vendor_dir):
        with open(self._stamp_path(vendor_dir), "w") as f:
            f.write(self.llvm_commit + "\n")


class LinuxSysroot(Component):
    name = "linux-sysroot"
    target_os = "linux"
    _URL = "https://commondatastorage.googleapis.com/chrome-linux-sysroot"
    _OUTPUT_DIR = os.path.join(THIRD_PARTY_DIR, "linux-sysroot")
    # Extract dir -> content sha256 (also the URL object name and the stamp value).
    _SYSROOTS = {
        "debian_bullseye_amd64-sysroot": "52d61d4446ffebfaa3dda2cd02da4ab4876ff237853f46d273e7f9b666652e1d",
        "debian_bullseye_arm64-sysroot": "c7176a4c7aacbf46bda58a029f39f79a68008d3dee6518f154dcf5161a5486d8",
    }

    def _stamp_path(self, sysroot_dir):
        return os.path.join(self._OUTPUT_DIR, sysroot_dir, ".stamp")

    def status(self, platform):
        for sysroot_dir, sha256 in self._SYSROOTS.items():
            stamp = self._stamp_path(sysroot_dir)
            if not os.path.exists(stamp):
                return Status.needs_fetch("missing")
            with open(stamp) as f:
                if f.read().strip() != sha256:
                    return Status.needs_fetch("stale")
        return Status.up_to_date()

    def fetch(self, platform):
        for sysroot_dir, sha256 in self._SYSROOTS.items():
            dest = os.path.join(self._OUTPUT_DIR, sysroot_dir)
            shutil.rmtree(dest, ignore_errors=True)
            os.makedirs(dest, exist_ok=True)
            _extract_tar(f"{self._URL}/{sha256}", dest)
            with open(self._stamp_path(sysroot_dir), "w") as f:
                f.write(sha256 + "\n")


class WindowsSysroot(Component):
    name = "win-sysroot"
    target_os = "windows"
    _OUTPUT_DIR = os.path.join(THIRD_PARTY_DIR, "win-sysroot")
    archs = ("x86_64", "aarch64")
    sdk_version: str | None = None
    crt_version: str | None = None

    def _stamp_path(self):
        return os.path.join(self._OUTPUT_DIR, ".stamp")

    def _stamp_value(self):
        version = subprocess.run(
            ["xwin", "--version"], capture_output=True, text=True, check=True
        ).stdout.strip()
        return (
            f"{version} archs={','.join(self.archs)} sdk={self.sdk_version} crt={self.crt_version}"
        )

    def status(self, platform):
        if shutil.which("xwin") is None:
            return Status.needs_fetch("xwin not installed")
        if not os.path.exists(self._stamp_path()):
            return Status.needs_fetch("missing")
        with open(self._stamp_path()) as f:
            fresh = f.read().strip() == self._stamp_value()
        return Status.up_to_date() if fresh else Status.needs_fetch("stale")

    def fetch(self, platform):
        if shutil.which("xwin") is None:
            sys.exit("xwin not found; install it (cargo install xwin / brew install xwin).")
        cmd = ["xwin", "--accept-license", "--temp", "--arch", ",".join(self.archs)]
        if self.sdk_version:
            cmd += ["--sdk-version", self.sdk_version]
        if self.crt_version:
            cmd += ["--crt-version", self.crt_version]
        cmd += [
            "splat",
            "--use-winsysroot-style",
            "--preserve-ms-arch-notation",
            "--include-debug-symbols",
            "--output",
            self._OUTPUT_DIR,
        ]
        subprocess.run(cmd, check=True)
        for required in ("VC", "Windows Kits"):
            if not os.path.isdir(os.path.join(self._OUTPUT_DIR, required)):
                sys.exit(f"xwin splat did not produce {required!r} under {self._OUTPUT_DIR}.")
        with open(self._stamp_path(), "w") as f:
            f.write(self._stamp_value() + "\n")


_DEV_TOOLS = ("clangd", "clang-tidy", "llvm-code-coverage", "llvmobjdump")
COMPONENTS = [
    # Essential: any build needs these.
    Gn(),
    Ninja(),
    ToolchainTool("clang"),
    LibcxxSource(),
    # Per-target: the compiler-rt runtime and sysroot for each cross-compile target.
    RuntimeLibrary("compiler-rt-mac", "clang-mac-runtime-library", "Mac", target_os="mac"),
    RuntimeLibrary("compiler-rt-win", "clang-win-runtime-library", "Win", target_os="windows"),
    LinuxRuntimeLibrary(),
    LinuxSysroot(),
    WindowsSysroot(),
    # Optional: dev tools, only when named or with --all.
    *(ToolchainTool(name, optional=True) for name in _DEV_TOOLS),
]


_GROUP_ORDER = ("essential", "mac", "linux", "windows", "optional")


def _default_components(platform):
    # A host build: the essentials plus the runtime/sysroot for the host's own OS.
    return [c for c in COMPONENTS if not c.optional and c.target_os in (None, platform.os_name)]


def _grouped_components():
    # Component names grouped by role, as aligned lines; shared by --help and errors.
    groups = {key: [] for key in _GROUP_ORDER}
    for component in COMPONENTS:
        groups["optional" if component.optional else component.target_os or "essential"].append(
            component.name
        )
    return [f"  {key + ':':<11}{', '.join(groups[key])}" for key in _GROUP_ORDER if groups[key]]


def _select(names, fetch_all, platform):
    if fetch_all:
        return COMPONENTS
    if not names:
        return _default_components(platform)
    by_name = {component.name: component for component in COMPONENTS}
    unknown = [name for name in names if name not in by_name]
    if unknown:
        listing = "\n".join(["choose from:", *_grouped_components()])
        sys.exit(f"unknown component(s): {', '.join(unknown)}\n{listing}")
    return [by_name[name] for name in names]


def _help_epilog():
    return "\n".join(["components:", *_grouped_components()])


def main():
    platform = Platform.host()
    parser = argparse.ArgumentParser(
        description="Sync pinned build dependencies.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=_help_epilog(),
    )
    default_names = ", ".join(c.name for c in _default_components(platform))
    parser.add_argument(
        "names",
        nargs="*",
        metavar="COMPONENT",
        help=f"Components to sync (default: {default_names}).",
    )
    parser.add_argument(
        "--all", dest="fetch_all", action="store_true", help="Sync every component."
    )
    parser.add_argument("--dry-run", action="store_true", help="Print the plan; fetch nothing.")
    args = parser.parse_args()

    for component in _select(args.names, args.fetch_all, platform):
        status = component.status(platform)
        if status.fresh:
            print(f"{component.name}: up to date")
        elif args.dry_run:
            print(f"{component.name}: would fetch ({status.reason})")
        else:
            component.fetch(platform)
            print(f"{component.name}: fetched")


if __name__ == "__main__":
    main()
