#include "shader_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Compile a shader from the provided source(s) */
GLint glueCompileShader(GLenum target, GLsizei count, const GLchar** sources, GLuint* shader) {
    GLint logLength, status;

    *shader = glCreateShader(target);
    glShaderSource(*shader, count, sources, NULL);
    glCompileShader(*shader);
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        printf("Shader compile log:\n%s", log);
        free(log);
    }

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        int i;

        printf("Failed to compile shader:\n");
        for (i = 0; i < count; i++) printf("%s", sources[i]);
    }

    return status;
}

/* Link a program with all currently attached shaders */
GLint glueLinkProgram(GLuint program) {
    GLint logLength, status;

    glLinkProgram(program);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetProgramInfoLog(program, logLength, &logLength, log);
        printf("Program link log:\n%s", log);
        free(log);
    }

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == 0) printf("Failed to link program %d", program);

    return status;
}

/* Validate a program (for i.e. inconsistent samplers) */
GLint glueValidateProgram(GLuint program) {
    GLint logLength, status;

    glValidateProgram(program);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar* log = (GLchar*)malloc(logLength);
        glGetProgramInfoLog(program, logLength, &logLength, log);
        printf("Program validate log:\n%s", log);
        free(log);
    }

    glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    if (status == 0) printf("Failed to validate program %d", program);

    return status;
}

/* Return named uniform location after linking */
GLint glueGetUniformLocation(GLuint program, const GLchar* uniformName) {
    GLint loc;

    loc = glGetUniformLocation(program, uniformName);

    return loc;
}

/* Convenience wrapper that compiles, links, enumerates uniforms and attribs */
GLint glueCreateProgram(const GLchar* vertSource, const GLchar* fragSource, GLsizei attribNameCt,
                        const GLchar** attribNames, const GLint* attribLocations,
                        GLsizei uniformNameCt, const GLchar** uniformNames,
                        GLint* uniformLocations, GLuint* program) {
    GLuint vertShader = 0, fragShader = 0, prog = 0, status = 1, i;

    prog = glCreateProgram();

    status *= glueCompileShader(GL_VERTEX_SHADER, 1, &vertSource, &vertShader);
    status *= glueCompileShader(GL_FRAGMENT_SHADER, 1, &fragSource, &fragShader);
    glAttachShader(prog, vertShader);
    glAttachShader(prog, fragShader);

    for (i = 0; i < attribNameCt; i++) {
        if (strlen(attribNames[i])) glBindAttribLocation(prog, attribLocations[i], attribNames[i]);
    }

    status *= glueLinkProgram(prog);
    status *= glueValidateProgram(prog);

    if (status) {
        for (i = 0; i < uniformNameCt; i++) {
            if (strlen(uniformNames[i]))
                uniformLocations[i] = glueGetUniformLocation(prog, uniformNames[i]);
        }
        *program = prog;
    }
    if (vertShader) glDeleteShader(vertShader);
    if (fragShader) glDeleteShader(fragShader);

    return status;
}
