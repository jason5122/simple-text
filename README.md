<h1 align="center">Simple Text - A GPU-accelerated, cross-platform text editor</h1>

<!-- <p align="center">
  <img alt="Simple Text - A GPU-accelerated, cross-platform text editor"
       src="docs/simple-text.png">
</p> -->

## Quickstart

```bash
python3 bin/fetch-gn
python3 bin/fetch-ninja
python3 bin/fetch-clang

bin/gn gen out/release --args='is_release=true'
bin/ninja -C out/release
```

## Dependencies

This project uses Clang/LLVM. The build system will search common paths to discover the LLVM install.

### macOS

Xcode Clang is unsupported at the moment. Please use Homebrew's version.

```bash
brew install llvm
```

### Fedora

```bash
dnf install clang llvm lld
```

## Development

```bash
# (Optional) Add LSP support for your editor. 
bin/ninja -C out/release -t compdb cc cxx objc objcxx > compile_commands.json
```
