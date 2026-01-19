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

```bash
bin/ninja -C out/release -t compdb cc cxx objc objcxx > compile_commands.json
```

## Cross Compilation

### macOS to Windows

```bash
brew install xwin
xwin --accept-license --arch x86_64 splat --output third_party/xwin
```

In `args.gn`, set `target_os` to `"win"`. Optionally, set `target_cpu`.

```
# Example for Windows x86_64.
target_os = "win"
target_cpu = "x64"
```

### Linux to Windows

> [!WARNING]
> TODO: Test the macOS to Windows approach here. It should work for Linux too.

### Windows to macOS/Linux

> [!CAUTION]
> Cross compilation on Windows is currently unexplored. Feel free to open an issue/PR if you're interested in this!
