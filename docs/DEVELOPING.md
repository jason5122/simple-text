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
