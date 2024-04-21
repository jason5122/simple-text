# TODO

## Priority

### GN

- **Consider replacing GN with a build system that works well on Windows.**
  - Good candidates include CMake and Meson.
- Fix Windows compilation not detecting changes in header files.

### UI Framework

- Reuse `//renderer` classes across windows.

### Linux

1. Load GTK 3 using `dlopen()` instead of adding it as a link-time dependency.

## Unordered

- Measure performance.
