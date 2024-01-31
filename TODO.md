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
3. Move `//util/CTFontUtil.h` into `//ui/mac/`.
4. Remove Objective-C from `//util`.
5. Add "Dependencies" section to `README.md` for macOS and Linux deps.
   - Linux:
     - `mesa-libEGL-devel` for `<EGL/egl.h>`
     - Reference [libdecor](https://gitlab.freedesktop.org/libdecor/libdecor) for a list of dependencies.

## Unordered

- Add FreeType renderer.
- Add HarfBuzz shaper.
- Measure performance.
