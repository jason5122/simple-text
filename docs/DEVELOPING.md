# Developing

## Setup

This repository uses Git submodules. Clone it recursively with `git clone --recursive`.

```bash
python3 bin/fetch-gn
python3 bin/fetch-ninja
python3 bin/fetch-llvm-toolchain

bin/gn gen out/release --args='is_release=true'
bin/ninja -C out/release
```

## LSP Support (Optional)

It is recommended to use a version of clangd that matches the LLVM toolchain:

```bash
python3 bin/fetch-llvm-toolchain --package=clangd
```

GN automatically generates `compile_commands.json` in the build directory (e.g., `out/release`). Configure clangd to search this directory.

## Cross Compilation

### macOS to Windows

```bash
brew install xwin
# Pick one of x86_64, aarch64, or both.
xwin --accept-license --arch x86_64,aarch64 splat --use-winsysroot-style --preserve-ms-arch-notation --output third_party/xwin
```

```bash
bin/gn args out/windows-x64
# When args.gn opens, set `target_os` and `target_cpu`. Example for Windows x86_64:
# target_os = "win"
# target_cpu = "x64"

bin/ninja -C out/windows-x64
```

### Linux to Windows

> [!WARNING]
> TODO: Test the macOS to Windows approach here. It should work for Linux too.

### Windows to macOS/Linux

> [!CAUTION]
> Cross compilation on Windows is currently unexplored. Feel free to open an issue/PR if you're interested in this!
