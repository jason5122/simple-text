#!/usr/bin/env python3
"""Fetch the libc++/libc++abi/llvm-libc sources our hermetic C++ runtime is built from.

We compile these into libc++ ourselves (see build/third_party/*/BUILD.gn), so the
source must be the exact LLVM commit the prebuilt toolchain was built from -- a
mismatched compiler and runtime is the kind of breakage that surfaces late. A
blobless + shallow + sparse fetch pulls only the subtrees we build (~20 MB) instead
of the 6 GB monorepo.
"""

import os
import shutil
import subprocess
import sys
import tempfile

from download import load_pins

LLVM_REPO = "https://github.com/llvm/llvm-project.git"

# The prebuilt toolchain's package version ends in -g<short-sha> of this commit;
# _check_toolchain_coherence() asserts they agree so the two can't drift apart.
LLVM_COMMIT = load_pins("libcxx")["llvm_commit"]

# Fetched source lives at the CONSUMER's checkout root (alongside third_party/
# llvm-toolchain), never inside the //build submodule -- writing into a vendored pin
# is the wrong model. The //build wrapper BUILD.gn references these by a GN arg, the
# same way it references the toolchain via clang_base_path.
DEST_ROOT = "third_party"

# Upstream subtree -> destination under third_party/. We take whole subtrees
# on purpose: libc++'s private impl (e.g. from_chars) reaches widely into llvm-libc's
# __support, so hand-picking headers silently re-breaks on the next roll. libunwind
# is unused, and only these four llvm-libc header trees are needed (not the rest of
# libc), so they stay out.
SUBTREES = {
    "libcxx/include": "libc++/include",
    "libcxx/src": "libc++/src",
    "libcxxabi/include": "libc++abi/include",
    "libcxxabi/src": "libc++abi/src",
    "libc/shared": "llvm-libc/shared",
    "libc/src/__support": "llvm-libc/src/__support",
    "libc/hdr": "llvm-libc/hdr",
    "libc/include": "llvm-libc/include",
}

# Top-level dirs under third_party/ that each get the upstream LICENSE copied in.
LICENSE_DESTS = ("libc++", "libc++abi", "llvm-libc")


def _check_toolchain_coherence():
    clang_revision = load_pins("llvm_toolchain")["clang_revision"]
    short_sha = clang_revision.rsplit("-g", 1)[-1]
    if not LLVM_COMMIT.startswith(short_sha):
        sys.exit(
            f"LLVM_COMMIT {LLVM_COMMIT} does not match toolchain {clang_revision} "
            f"(-g{short_sha}); roll both to the same LLVM commit."
        )


def _git(args, cwd):
    subprocess.run(["git", *args], cwd=cwd, check=True)


def _sparse_fetch(work_dir):
    _git(["init", "-q"], work_dir)
    _git(["remote", "add", "origin", LLVM_REPO], work_dir)
    # --filter=blob:none downloads trees but no file contents; the sparse set below
    # then limits which blobs the checkout actually pulls.
    _git(["fetch", "-q", "--depth", "1", "--filter=blob:none", "origin", LLVM_COMMIT], work_dir)
    _git(["sparse-checkout", "set", "--cone", *sorted(SUBTREES)], work_dir)
    _git(["checkout", "-q", "FETCH_HEAD"], work_dir)


def _vendor(work_dir):
    for upstream, dest in SUBTREES.items():
        dst = os.path.join(DEST_ROOT, dest)
        # Replace only the vendored subtree, leaving sibling hand-written files
        # (BUILD.gn, __config_site, __assertion_handler) in place.
        shutil.rmtree(dst, ignore_errors=True)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copytree(os.path.join(work_dir, upstream), dst)

    license_src = os.path.join(work_dir, "LICENSE.TXT")
    for dest in LICENSE_DESTS:
        shutil.copy(license_src, os.path.join(DEST_ROOT, dest, "LICENSE.TXT"))


def main():
    if shutil.which("git") is None:
        sys.exit("git is required to fetch libc++ sources.")
    _check_toolchain_coherence()

    # Navigate to project root (//build/deps -> //).
    os.chdir(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))

    with tempfile.TemporaryDirectory() as work_dir:
        print(f"Fetching libc++ sources at {LLVM_COMMIT[:12]} ...")
        _sparse_fetch(work_dir)
        _vendor(work_dir)
    print(f"Vendored {len(SUBTREES)} subtrees into {DEST_ROOT}/")


if __name__ == "__main__":
    main()
