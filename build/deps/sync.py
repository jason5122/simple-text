#!/usr/bin/env python3
"""Sync pinned dependencies for a host build (default) or named cross-compile targets.

  python3 build/deps/sync.py                      # host build
  python3 build/deps/sync.py --target linux       # + linux compiler-rt & both sysroots
  python3 build/deps/sync.py --all-targets --with clangd,clang-tidy
  python3 build/deps/sync.py --dry-run            # print the plan, fetch nothing

Pins live in deps.json. Binaries (gn/ninja) are verified by per-platform SHA256; the
toolchain records installed packages in third_party/llvm-toolchain/.installed.json; and
libc++ source + Linux sysroots use content/version stamps. Windows and macOS sysroots
are not managed here yet (Phase 2).
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys

from download import get_os_cpu, load_pins
from fetch_llvm_toolchain import PLATFORM_BY_OS_CPU

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))  # //build/deps -> //

TARGET_OSES = ("mac", "linux", "win")
DEV_TOOLS = ("clangd", "clang-tidy", "llvm-code-coverage", "llvmobjdump", "libclang",
             "translation_unit")

# Platform detection speaks mac|linux|windows x x86_64|arm64; the CLI, deps.json keys,
# and GN all speak mac|linux|win x x64|arm64. This is the one crossing point.
_PY2GN_OS = {"mac": "mac", "linux": "linux", "windows": "win"}
_PY2GN_CPU = {"x86_64": "x64", "arm64": "arm64"}

TOOLCHAIN_DIR = os.path.join(ROOT, "third_party", "llvm-toolchain")
INSTALLED_FILE = os.path.join(TOOLCHAIN_DIR, ".installed.json")


def host_gn():
    os_name, cpu = get_os_cpu()
    return _PY2GN_OS[os_name], _PY2GN_CPU[cpu]


def _run(script, *args):
    subprocess.run([sys.executable, os.path.join(SCRIPT_DIR, script), *args], check=True)


def _split_csv(values):
    return [item for value in values for item in value.split(",") if item]


def _sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _stamp_matches(path, value):
    if not os.path.exists(path):
        return False
    with open(path) as f:
        return f.read().strip() == value


def _write_stamp(path, value):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(value + "\n")


# ---- binaries (gn, ninja): pinned by per-platform sha256 --------------------

def _sync_binary(name, script, gn_platform, dry_run):
    expected = load_pins(name)["sha256"].get(gn_platform)
    exe = name + (".exe" if gn_platform.startswith("win") else "")
    path = os.path.join(ROOT, "bin", exe)

    if expected and os.path.exists(path) and _sha256(path) == expected:
        print(f"{name}: up to date")
        return True
    if dry_run:
        print(f"{name}: would fetch")
        return True

    _run(script)
    actual = _sha256(path)
    if expected is None:
        print(f'{name}: no pinned sha256 for {gn_platform}; add to deps.json:\n'
              f'    "{gn_platform}": "{actual}"')
        return True
    if actual != expected:
        print(f"{name}: checksum mismatch\n  expected {expected}\n  actual   {actual}",
              file=sys.stderr)
        return False
    print(f"{name}: fetched and verified")
    return True


# ---- toolchain: per-package installed manifest ------------------------------

def _package_version():
    tc = load_pins("llvm_toolchain")
    return f"{tc['clang_revision']}-{tc['clang_sub_revision']}"


def _read_installed():
    if os.path.exists(INSTALLED_FILE):
        with open(INSTALLED_FILE) as f:
            return json.load(f)
    # Migration: seed from the legacy monolithic stamp (clang + host runtime) so an
    # already-populated toolchain isn't needlessly refetched on the first sync.
    legacy = os.path.join(TOOLCHAIN_DIR, ".stamp")
    if os.path.exists(legacy):
        with open(legacy) as f:
            version = f.read().strip()
        host_os, _ = host_gn()
        return {"clang": version, f"compiler-rt-{host_os}": version}
    return {}


def _sync_toolchain(packages, dry_run):
    version = _package_version()
    installed = _read_installed()
    needed = [p for p in packages if installed.get(p) != version]
    if not needed:
        print(f"toolchain: up to date ({', '.join(packages)})")
        return True
    if dry_run:
        print(f"toolchain: would fetch {', '.join(needed)}")
        return True

    _run("fetch_llvm_toolchain.py", *needed)
    for package in needed:
        installed[package] = version
    os.makedirs(TOOLCHAIN_DIR, exist_ok=True)
    with open(INSTALLED_FILE, "w") as f:
        json.dump(installed, f, indent=2, sort_keys=True)
        f.write("\n")
    print(f"toolchain: fetched {', '.join(needed)}")
    return True


# ---- libc++ source ----------------------------------------------------------

def _sync_libcxx(dry_run):
    commit = load_pins("libcxx")["llvm_commit"]
    stamp = os.path.join(ROOT, "third_party", ".libcxx.stamp")
    if _stamp_matches(stamp, commit):
        print("libcxx: up to date")
        return True
    if dry_run:
        print("libcxx: would fetch")
        return True
    _run("fetch_libcxx.py")
    _write_stamp(stamp, commit)
    print(f"libcxx: fetched ({commit[:12]})")
    return True


# ---- sysroots ---------------------------------------------------------------

def _sync_linux_sysroot(dry_run):
    for name, info in load_pins("linux_sysroot").items():
        stamp = os.path.join(ROOT, "third_party", "linux-sysroot", info["dir"], ".stamp")
        if _stamp_matches(stamp, info["sha256"]):
            print(f"linux_sysroot/{name}: up to date")
            continue
        if dry_run:
            print(f"linux_sysroot/{name}: would fetch")
            continue
        _run("fetch_linux_sysroot.py", name)
        _write_stamp(stamp, info["sha256"])
        print(f"linux_sysroot/{name}: fetched")
    return True


def _sync_sysroots(targets, dry_run):
    ok = True
    for target in sorted(targets):
        if target == "linux":
            ok = _sync_linux_sysroot(dry_run) and ok
        elif target == "mac":
            pass  # mac uses the system SDK (find_sdk.py); hermetic SDK is Phase 2.
        elif target == "win":
            print("win_sysroot: not managed yet (Phase 2: fetch_win_sysroot.py wraps xwin)")
    return ok


# ---- driver -----------------------------------------------------------------

def _resolve(args):
    host_os, host_cpu = host_gn()
    supported = {(_PY2GN_OS[o], _PY2GN_CPU[c]) for (o, c) in PLATFORM_BY_OS_CPU}
    if (host_os, host_cpu) not in supported:
        sys.exit(f"{host_os}-{host_cpu} has no prebuilt host clang; it is a cross-compile "
                 f"target only. Run sync on a supported host: "
                 f"{', '.join(sorted(f'{o}-{c}' for o, c in supported))}.")

    targets = _split_csv(args.target)
    for target in targets:
        if target not in TARGET_OSES:
            sys.exit(f"unknown --target {target!r}; choose from: {', '.join(TARGET_OSES)}")
    tools = _split_csv(args.tools)
    for tool in tools:
        if tool not in DEV_TOOLS:
            sys.exit(f"unknown --with tool {tool!r}; choose from: {', '.join(DEV_TOOLS)}")

    target_set = set(TARGET_OSES) if args.all_targets else ({host_os} | set(targets))
    packages = ["clang"] + [f"compiler-rt-{os_}" for os_ in sorted(target_set)] + tools
    return f"{host_os}-{host_cpu}", target_set, packages, tools


def main():
    parser = argparse.ArgumentParser(
        description="Sync pinned build dependencies.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="targets: " + ", ".join(TARGET_OSES) + "\ndev tools (--with): " + ", ".join(DEV_TOOLS),
    )
    parser.add_argument("--target", action="append", default=[], metavar="OS",
                        help="Also set up to cross-compile for this OS (repeatable / comma-list).")
    parser.add_argument("--all-targets", action="store_true", help="Set up all target OSes.")
    parser.add_argument("--with", dest="tools", action="append", default=[], metavar="TOOL",
                        help="Opt-in toolchain dev tool(s) (repeatable / comma-list).")
    parser.add_argument("--dry-run", action="store_true", help="Print the plan; fetch nothing.")
    args = parser.parse_args()

    host, targets, packages, tools = _resolve(args)
    print(f"host {host}; targets {', '.join(sorted(targets))}"
          + (f"; tools {', '.join(tools)}" if tools else ""))

    ok = True
    ok = _sync_binary("gn", "fetch_gn.py", host, args.dry_run) and ok
    ok = _sync_binary("ninja", "fetch_ninja.py", host, args.dry_run) and ok
    ok = _sync_toolchain(packages, args.dry_run) and ok
    ok = _sync_libcxx(args.dry_run) and ok
    ok = _sync_sysroots(targets, args.dry_run) and ok
    if not ok:
        sys.exit(1)


if __name__ == "__main__":
    main()
