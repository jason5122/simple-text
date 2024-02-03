# TODO

## Priority

1. Set up **pkg-config** for Linux.
   - Reference Chromium:
     - `//build/config/linux/pkg_config.gni`
     - `//build/config/linux/pkg-config.py`
     - `//ui/gtk:gtk_config`
2. Set up testing in GN.
   - (Optional) Use a testing framework.
   - Reference Chromium:
     - `//base/test:test_config`, `//base/test:test_support`, etc.
3. Remove Objective-C from `//util`.
4. Add "Dependencies" section to `README.md` for macOS and Linux deps.
   - Linux:
     - `mesa-libEGL-devel` for `<EGL/egl.h>`
     - Reference [libdecor](https://gitlab.freedesktop.org/libdecor/libdecor) for a list of dependencies.
5. Add delegates to Objective-C code.

## Unordered

- Add FreeType renderer.
- Add HarfBuzz shaper.
- Measure performance.
