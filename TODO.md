# TODO

## Priority

1. Add "Dependencies" section to `README.md` for macOS and Linux deps.
   - Linux:
     - `mesa-libEGL-devel` for `<EGL/egl.h>`
     - Reference [libdecor](https://gitlab.freedesktop.org/libdecor/libdecor) for a list of dependencies.
2. Add delegates to Objective-C code.

## Unordered

- Measure performance.
- Consider switching to Bazel
  - [Shaka Player discussion](https://github.com/shaka-project/shaka-player-embedded/issues/19) regarding this subject
- If going back to `NSOpenGLView`, implement a custom version.
  - [Custom OpenGL view docs](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_drawing/opengl_drawing.html#//apple_ref/doc/uid/TP40001987-CH404-SW3)
  - [Custom OpenGL sample code](https://developer.apple.com/library/archive/samplecode/BasicMultiGPUSample/Listings/MyOpenGLView_m.html)
