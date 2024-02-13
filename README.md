<h1 align="center">Simple Text - A GPU-accelerated, cross-platform text editor</h1>

<p align="center">
  <img alt="Simple Text - A GPU-accelerated, cross-platform text editor"
       src="docs/simple-text.png">
</p>

## Dependencies

This project uses the GN meta-build system. Binaries are available [here](https://gn.googlesource.com/gn#getting-a-binary).

### macOS

`brew install ninja llvm`

## Building

```
gn gen out/Default
ninja -C out/Default
```
