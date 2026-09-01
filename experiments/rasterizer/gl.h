#pragma once

#include "build/build_config.h"

// One OpenGL 4.1 core API for the shared renderer code. macOS declares the entry points in its own
// header; on Windows opengl32.dll only exports 1.1, so the rest come from //GL, which resolves
// them through wglGetProcAddress into `namespace gl`. The using-directive is what lets atlas.cc
// and win/gl_renderer.cc spell the calls the same way on both platforms; it is the reason this
// header exists.

#if BUILDFLAG(IS_MAC)
#include <OpenGL/gl3.h>
#elif BUILDFLAG(IS_WIN)
#include "gl/gl.h"
using namespace gl;  // NOLINT(google-build-using-namespace)
#endif
