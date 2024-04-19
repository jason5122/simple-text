# TODO

## Priority

### GN

- Fix Windows compilation not detecting changes in header files.
  - Look into `precompiled_header_type` variable.

### UI Framework

- Reuse `//renderer` classes across windows.
- Delete `EditorWindow` resources on _any_ form of close, not just through keybinds.
  - Specifically, keep track of closing via the close button or external programs.
- Win: Keep track of windows being closed through `WM_DESTROY` instead of during keybinds.

### Linux

1. Load GTK 3 using `dlopen()` instead of adding it as a link-time dependency.

## Unordered

- Measure performance.
- Consider switching to Bazel
  - [Shaka Player discussion](https://github.com/shaka-project/shaka-player-embedded/issues/19) regarding this subject
