#ifndef SHADERUTIL_H
#define SHADERUTIL_H

#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

/* Shader Utilities */
GLint glueCompileShader(GLenum target, GLsizei count, const GLchar** sources,
                        GLuint* shader);
GLint glueLinkProgram(GLuint program);
GLint glueValidateProgram(GLuint program);
GLint glueGetUniformLocation(GLuint program, const GLchar* name);

/* Shader Conveniences */
GLint glueCreateProgram(const GLchar* vertSource, const GLchar* fragSource,
                        GLsizei attribNameCt, const GLchar** attribNames,
                        const GLint* attribLocations, GLsizei uniformNameCt,
                        const GLchar** uniformNames, GLint* uniformLocations,
                        GLuint* program);

#ifdef __cplusplus
}
#endif

#endif /* SHADERUTIL_H */
