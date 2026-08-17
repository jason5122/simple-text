# Developing

Table of Contents:

1. [Setup](#setup)
1. [Editor support (clangd)](#editor-support-clangd)
1. [Build configuration](#build-configuration)
1. [Cross-compiling](#cross-compiling)

## Setup

This repo uses Git submodules — clone with `git clone --recursive` (or, in an existing
clone, run `git submodule update --init --recursive`).

```bash
python3 build/sync_deps.py   # fetch host build dependencies (clang, gn, ninja, ...)
bin/gn gen out/debug         # create a build directory
bin/ninja -C out/debug       # build
```

`sync_deps.py` with no arguments fetches what a host build needs; pass `--help` to see
every component, or `--all` to fetch them all.

## Editor support (clangd)

Fetch a clangd that matches the toolchain:

```bash
python3 build/sync_deps.py clangd
```

GN writes a `compile_commands.json` into each build directory (e.g. `out/debug`); point
clangd at it.

## Build configuration

Builds are configured with GN "args", set per output directory. New to GN? Its
[quick-start guide](https://gn.googlesource.com/gn/+/HEAD/docs/quick_start.md) is a
good short intro.

Set args when generating, or edit them later with `bin/gn args out/<dir>` (opens an editor):

```bash
bin/gn gen out/release --args='is_release=true'
```

| Arg          | Values                        | Meaning                  |
| ------------ | ----------------------------- | ------------------------ |
| `is_release` | `true` / `false`              | optimized build          |
| `target_os`  | `"mac"` / `"linux"` / `"win"` | cross-compile target OS  |
| `target_cpu` | `"x64"` / `"arm64"`           | cross-compile target CPU |

## Cross-compiling

Fetch the target's runtime and sysroot, then set `target_os` / `target_cpu`. The blocks
below are **templates** — rename the `out/` dir and pick the `target_cpu` you want.

### → Windows

`win-sysroot` uses [xwin](https://github.com/Jake-Shadle/xwin); install it first (`brew install xwin` or `cargo install xwin`).

```bash
python3 build/sync_deps.py compiler-rt-win win-sysroot
bin/gn gen out/win-x64 --args='target_os="win" target_cpu="x64"'
bin/ninja -C out/win-x64
```

### → Linux

```bash
python3 build/sync_deps.py compiler-rt-linux linux-sysroot
bin/gn gen out/linux-x64 --args='target_os="linux" target_cpu="x64"'
bin/ninja -C out/linux-x64
```

### → macOS

On a Mac this is just a normal build (see [Setup](#setup)) — the SDK is found
automatically. The steps here are only for building macOS binaries **from Linux or
Windows**.

> [!IMPORTANT]
> Apple's license forbids us from mirroring the macOS SDK, so you supply it yourself
> (an Apple Account is required to download Xcode).

1. Download Xcode 15.0 from [Xcode Releases](https://xcodereleases.com/?q=15.0&scope=release).
2. Unpack `Xcode.app` into `third_party/mac-sysroot/`.

```bash
python3 build/sync_deps.py compiler-rt-mac
bin/gn gen out/mac-arm64 --args='
  target_os = "mac"
  target_cpu = "arm64"
  mac_sdk_path = "//third_party/mac-sysroot/Xcode.app"
'
bin/ninja -C out/mac-arm64
```
