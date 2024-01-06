# MetalApp

## Building

This project uses the GN meta-build system. Binaries are available [here](https://gn.googlesource.com/gn#getting-a-binary). This was done to avoid relying on Xcode! GN is also just fast and very nice to work with.

The build system, Ninja, can be installed with `brew install ninja`.

The archiver `llvm-ar` is used for linking static libraries and is part of `brew install llvm`.

### Setting up the build

It is highly recommended to code sign the app! This prevents macOS from asking for Accessiblity/Screen Recording permissions on every recompile.

To do this, find your ID using `security find-identity -v -p codesigning`. Then,

```
gn args out
```

and paste your ID in the editor as

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

This generates `MetalApp.app` in the `out/` directory.

## Viewing logs

```
log stream --predicate 'subsystem contains "com.jason5122.metal-app"' --style compact
```

You can also use `Console.app`, but I prefer viewing them in the terminal so I use this command.
