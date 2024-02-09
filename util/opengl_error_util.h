#pragma once

#include <cstdio>

static inline void glPrintError() {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error: %d", error);
    }
}
