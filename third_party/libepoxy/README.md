# libepoxy

## Modifications

- Removed all other directories besides `src/`, `include/`, and `registry/`.
- Removed documentation, tests, and READMEs.
- Added a dummy `config.h` in `include/`, defining Meson's config values in `BUILD.gn` instead.
- Removed `__declspec(dllimport)` from `include/epoxy/gl.h` (line 75) and `include/epoxy/common.h` (line 42) to support Windows static compiles.
  - [PR addressing this issue](https://github.com/anholt/libepoxy/pull/296)
