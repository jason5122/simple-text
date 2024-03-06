<h1 align="center">Simple Text - A GPU-accelerated, cross-platform text editor</h1>

<p align="center">
  <img alt="Simple Text - A GPU-accelerated, cross-platform text editor"
       src="docs/simple-text.png">
</p>

## Progress Chart

| Feature              | macOS          | Linux         | Windows          |
| -------------------- | -------------- | ------------- | ---------------- |
| Text Rasterizer      | ✅ (Core Text) | ✅ (FreeType) | ❌ (DirectWrite) |
| Text Shaper          | ✅ (Core Text) | ✅ (HarfBuzz) | ❌ (???)         |
| Window creation      | ✅ (Cocoa)     | ✅ (GTK)      | ❌ (Win32)       |
| OpenGL context       | ✅ (Cocoa)     | ✅ (libepoxy) | ❌               |
| Keyboard/mouse input | ✅             | 🚧            | ❌               |
| Tabs                 | ❌             | ❌            | ❌               |
| Popups/Dialogs       | ❌             | ❌            | ❌               |

## Dependencies

This project uses the GN meta-build system. Binaries are available [here](https://gn.googlesource.com/gn#getting-a-binary).

### macOS

`brew install ninja llvm`

### Fedora

`dnf install gn ninja-build clang llvm lld`

## Building

```
gn gen out/Default
ninja -C out/Default
```

## Why Simple Text?

> [!NOTE]\
> These are features that are important from _my point of view_!\
> Someone who appreciates other features (e.g., AI integration, plugins) would have a different chart.

| Editor             | GUI? | Native GUI? | Cross-platform? | Fast? | Open source? | Tree-sitter support? | LSP support? | Simple codebase? | Notes                                                                        |
| ------------------ | ---- | ----------- | --------------- | ----- | ------------ | -------------------- | ------------ | ---------------- | ---------------------------------------------------------------------------- |
| Simple Text        | ✅   | ✅          | ✅              | ✅    | ✅           | ✅                   | ✅           | ✅               |                                                                              |
| Sublime Text       | ✅   | ✅          | ✅              | ✅    | ❌           | ❌                   | ✅           | N/A              | Very nearly perfect!                                                         |
| Visual Studio Code | ✅   | ❌          | ✅              | ❌    | ✅           | ❌                   | ✅           | ❌               |                                                                              |
| Zed                | ✅   | ❌          | ❌              | ✅    | ✅           | ✅                   | ✅           | ❌               | Contains a _lot_ of arguably unnecessary features, such as AI and voice chat |
| Lapce              | ✅   | ❌          | ✅              | ✅    | ✅           | ✅                   | ✅           | ❌               | GUI doesn't feel super polished, at least on macOS (e.g., blurry fonts)      |
| Neovim             | ❌   | N/A         | ✅              | ✅    | ✅           | ✅                   | ✅           | ❌               |                                                                              |
