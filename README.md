# Simple Text

## Building

This project uses the GN meta-build system. Binaries are available [here](https://gn.googlesource.com/gn#getting-a-binary). This was done to avoid relying on Xcode. GN is also just fast and very nice to work with.

The build system, Ninja, can be installed with `brew install ninja`.

The archiver `llvm-ar` is used for linking static libraries and is part of `brew install llvm`.

### Setting up the build

```
gn args out
```

Optionally, you can code sign the app. Find your ID using `security find-identity -v -p codesigning` and add it to `args.gn` as

```
code_signing_identity = "your 40 hexadecimal digits"
```

Next, run

```
gn gen out
```

### Compiling/recompiling

```
ninja -C out
```

This generates `Simple Text.app` in the `out/` directory.
