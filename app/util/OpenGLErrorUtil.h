static inline void glPrintError() {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        logDefault(@"OpenGLErrorUtil", @"OpenGL error: %d", error);
    }
}
