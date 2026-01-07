#include "gl/gl.h"
#include "gl/loader.h"

namespace gl {

#define LOAD_GL_FUNC(FP) FP = reinterpret_cast<decltype(FP)>(internal::load_proc_address(#FP))

void load_global_function_pointers() {
    // 1.0
    LOAD_GL_FUNC(glClear);
    LOAD_GL_FUNC(glClearColor);
    LOAD_GL_FUNC(glDepthMask);
    LOAD_GL_FUNC(glEnable);
    LOAD_GL_FUNC(glGetError);
    LOAD_GL_FUNC(glGetIntegerv);
    LOAD_GL_FUNC(glViewport);
    LOAD_GL_FUNC(glPixelStorei);
    LOAD_GL_FUNC(glTexImage2D);
    LOAD_GL_FUNC(glTexParameteri);
    LOAD_GL_FUNC(glBlendFunc);
    LOAD_GL_FUNC(glScissor);
    LOAD_GL_FUNC(glDisable);
    LOAD_GL_FUNC(glFlush);

    // 1.1
    LOAD_GL_FUNC(glGenTextures);
    LOAD_GL_FUNC(glDeleteTextures);
    LOAD_GL_FUNC(glBindTexture);
    LOAD_GL_FUNC(glTexSubImage2D);
    LOAD_GL_FUNC(glDrawArrays);

    // 1.3
    LOAD_GL_FUNC(glActiveTexture);

    // 1.4
    LOAD_GL_FUNC(glBlendFuncSeparate);

    // 1.5
    LOAD_GL_FUNC(glGenBuffers);
    LOAD_GL_FUNC(glBindBuffer);
    LOAD_GL_FUNC(glBufferData);
    LOAD_GL_FUNC(glBufferSubData);
    LOAD_GL_FUNC(glDeleteBuffers);

    // 2.0
    LOAD_GL_FUNC(glAttachShader);
    LOAD_GL_FUNC(glCompileShader);
    LOAD_GL_FUNC(glCreateProgram);
    LOAD_GL_FUNC(glCreateShader);
    LOAD_GL_FUNC(glDeleteProgram);
    LOAD_GL_FUNC(glDeleteShader);
    LOAD_GL_FUNC(glLinkProgram);
    LOAD_GL_FUNC(glShaderSource);
    LOAD_GL_FUNC(glGetShaderInfoLog);
    LOAD_GL_FUNC(glGetProgramInfoLog);
    LOAD_GL_FUNC(glGetShaderiv);
    LOAD_GL_FUNC(glGetProgramiv);
    LOAD_GL_FUNC(glEnableVertexAttribArray);
    LOAD_GL_FUNC(glVertexAttribPointer);
    LOAD_GL_FUNC(glUseProgram);
    LOAD_GL_FUNC(glGetUniformLocation);
    LOAD_GL_FUNC(glUniform2f);
    LOAD_GL_FUNC(glUniform1f);
    LOAD_GL_FUNC(glUniform1i);
    LOAD_GL_FUNC(glGetAttribLocation);

    // 3.0
    LOAD_GL_FUNC(glBindVertexArray);
    LOAD_GL_FUNC(glGenVertexArrays);
    LOAD_GL_FUNC(glDeleteVertexArrays);
    LOAD_GL_FUNC(glVertexAttribIPointer);

    // 3.1
    LOAD_GL_FUNC(glDrawElementsInstanced);

    // 3.3
    LOAD_GL_FUNC(glVertexAttribDivisor);
}

}  // namespace gl
