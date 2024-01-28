#pragma once

#import "util/log_util.h"

static inline void glPrintError() {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LogDefault("OpenGLErrorUtil", "OpenGL error: %d", error);
    }
}
