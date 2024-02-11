#pragma once

#include <cstdio>

static inline void glPrintError() {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        if (error == GL_INVALID_VALUE) {
            fprintf(stderr, "OpenGL error: GL_INVALID_VALUE\n");
        } else if (error == GL_INVALID_OPERATION) {
            fprintf(stderr, "OpenGL error: GL_INVALID_OPERATION\n");
        } else {
            fprintf(stderr, "OpenGL unknown error: %d\n", error);
        }
    }
}
