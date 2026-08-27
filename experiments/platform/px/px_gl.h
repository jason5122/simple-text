// Platform-specific access to the GL entry points a drawing layer needs.
//
// macOS: the CAOpenGLLayer's context is 3.2 core, so the SDK's gl3.h symbols are linkable
// directly.
//
// Windows: opengl32.dll exports only GL 1.1, and everything newer has to come through
// wglGetProcAddress. ST does exactly this -- its import table lists 23 GL 1.1 entry points
// (glDrawArrays, glTexImage2D, glScissor, glViewport, glFlush, ...) and the binary carries the
// strings "OPENGL32.dll" and "wglGetProcAddress" for the rest.
//
// px_gl_has_shaders() reports whether the modern pipeline actually resolved. It can be false on a
// machine whose GL is a software rasteriser stuck at 1.1, which is a real possibility inside a VM,
// so drawing code should degrade rather than crash.

#pragma once

#if defined(__APPLE__)

#include <OpenGL/gl3.h>

inline bool px_gl_has_shaders() { return true; }

#elif defined(_WIN32)

#include <windows.h>

// The Windows SDK's <GL/gl.h> is deliberately not included: this repository has its own GL/gl.h at
// its root, and the root is on the include path, so the angle-bracket include resolves to the
// wrong header. Declaring the GL 1.1 surface directly keeps this file self-contained and
// unambiguous. opengl32.lib supplies the symbols.

using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLbyte = signed char;
using GLshort = short;
using GLint = int;
using GLsizei = int;
using GLubyte = unsigned char;
using GLushort = unsigned short;
using GLuint = unsigned int;
using GLfloat = float;
using GLclampf = float;
using GLdouble = double;
using GLvoid = void;
using GLchar = char;
using GLsizeiptr = signed long long;
using GLintptr = signed long long;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_ONE 1
#define GL_EQUAL 0x0202
#define GL_ALWAYS 0x0207
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_STENCIL_TEST 0x0B90
#define GL_FLOAT 0x1406
#define GL_VERSION 0x1F02
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_ONE_MINUS_SRC_ALPHA 0x0303

// GL 1.1, linked directly.
extern "C" {
__declspec(dllimport) void APIENTRY glClear(GLbitfield mask);
__declspec(dllimport) void APIENTRY glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
__declspec(dllimport) void APIENTRY glClearStencil(GLint value);
__declspec(dllimport) void APIENTRY glBlendFunc(GLenum source, GLenum destination);
__declspec(dllimport) void APIENTRY glColorMask(GLboolean r,
                                                GLboolean g,
                                                GLboolean b,
                                                GLboolean a);
__declspec(dllimport) void APIENTRY glDisable(GLenum cap);
__declspec(dllimport) void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count);
__declspec(dllimport) void APIENTRY glEnable(GLenum cap);
__declspec(dllimport) void APIENTRY glFlush(void);
__declspec(dllimport) void APIENTRY glGenTextures(GLsizei count, GLuint* textures);
__declspec(dllimport) void APIENTRY glBindTexture(GLenum target, GLuint texture);
__declspec(dllimport) const GLubyte* APIENTRY glGetString(GLenum name);
__declspec(dllimport) void APIENTRY glScissor(GLint x, GLint y, GLsizei w, GLsizei h);
__declspec(dllimport) void APIENTRY glStencilFunc(GLenum function, GLint reference, GLuint mask);
__declspec(dllimport) void APIENTRY glStencilMask(GLuint mask);
__declspec(dllimport) void APIENTRY glStencilOp(GLenum fail, GLenum depth_fail, GLenum depth_pass);
__declspec(dllimport) void APIENTRY glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
}

// Constants GL 1.1 predates.
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_STREAM_DRAW 0x88E0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_RGBA32F 0x8814

// Resolved by the platform layer once the GL context is current. Named px_gl* and then macro'd
// onto the standard spellings, so drawing code reads the same on both platforms.
using PFN_glCreateShader = GLuint(APIENTRY*)(GLenum);
using PFN_glShaderSource = void(APIENTRY*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using PFN_glCompileShader = void(APIENTRY*)(GLuint);
using PFN_glGetShaderiv = void(APIENTRY*)(GLuint, GLenum, GLint*);
using PFN_glGetShaderInfoLog = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFN_glDeleteShader = void(APIENTRY*)(GLuint);
using PFN_glCreateProgram = GLuint(APIENTRY*)(void);
using PFN_glAttachShader = void(APIENTRY*)(GLuint, GLuint);
using PFN_glBindAttribLocation = void(APIENTRY*)(GLuint, GLuint, const GLchar*);
using PFN_glLinkProgram = void(APIENTRY*)(GLuint);
using PFN_glGetProgramiv = void(APIENTRY*)(GLuint, GLenum, GLint*);
using PFN_glGetProgramInfoLog = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFN_glDeleteProgram = void(APIENTRY*)(GLuint);
using PFN_glUseProgram = void(APIENTRY*)(GLuint);
using PFN_glGetUniformLocation = GLint(APIENTRY*)(GLuint, const GLchar*);
using PFN_glUniform2f = void(APIENTRY*)(GLint, GLfloat, GLfloat);
using PFN_glUniform1i = void(APIENTRY*)(GLint, GLint);
using PFN_glActiveTexture = void(APIENTRY*)(GLenum);
using PFN_glGenVertexArrays = void(APIENTRY*)(GLsizei, GLuint*);
using PFN_glBindVertexArray = void(APIENTRY*)(GLuint);
using PFN_glGenBuffers = void(APIENTRY*)(GLsizei, GLuint*);
using PFN_glBindBuffer = void(APIENTRY*)(GLenum, GLuint);
using PFN_glBufferData = void(APIENTRY*)(GLenum, GLsizeiptr, const void*, GLenum);
using PFN_glBufferSubData = void(APIENTRY*)(GLenum, GLintptr, GLsizeiptr, const void*);
using PFN_glTexBuffer = void(APIENTRY*)(GLenum, GLenum, GLuint);
using PFN_glDrawArraysInstanced = void(APIENTRY*)(GLenum, GLint, GLsizei, GLsizei);
using PFN_glEnableVertexAttribArray = void(APIENTRY*)(GLuint);
using PFN_glVertexAttribPointer =
    void(APIENTRY*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);

extern PFN_glCreateShader px_glCreateShader;
extern PFN_glShaderSource px_glShaderSource;
extern PFN_glCompileShader px_glCompileShader;
extern PFN_glGetShaderiv px_glGetShaderiv;
extern PFN_glGetShaderInfoLog px_glGetShaderInfoLog;
extern PFN_glDeleteShader px_glDeleteShader;
extern PFN_glCreateProgram px_glCreateProgram;
extern PFN_glAttachShader px_glAttachShader;
extern PFN_glBindAttribLocation px_glBindAttribLocation;
extern PFN_glLinkProgram px_glLinkProgram;
extern PFN_glGetProgramiv px_glGetProgramiv;
extern PFN_glGetProgramInfoLog px_glGetProgramInfoLog;
extern PFN_glDeleteProgram px_glDeleteProgram;
extern PFN_glUseProgram px_glUseProgram;
extern PFN_glGetUniformLocation px_glGetUniformLocation;
extern PFN_glUniform2f px_glUniform2f;
extern PFN_glUniform1i px_glUniform1i;
extern PFN_glActiveTexture px_glActiveTexture;
extern PFN_glGenVertexArrays px_glGenVertexArrays;
extern PFN_glBindVertexArray px_glBindVertexArray;
extern PFN_glGenBuffers px_glGenBuffers;
extern PFN_glBindBuffer px_glBindBuffer;
extern PFN_glBufferData px_glBufferData;
extern PFN_glBufferSubData px_glBufferSubData;
extern PFN_glTexBuffer px_glTexBuffer;
extern PFN_glDrawArraysInstanced px_glDrawArraysInstanced;
extern PFN_glEnableVertexAttribArray px_glEnableVertexAttribArray;
extern PFN_glVertexAttribPointer px_glVertexAttribPointer;

bool px_gl_has_shaders();

#define glCreateShader px_glCreateShader
#define glShaderSource px_glShaderSource
#define glCompileShader px_glCompileShader
#define glGetShaderiv px_glGetShaderiv
#define glGetShaderInfoLog px_glGetShaderInfoLog
#define glDeleteShader px_glDeleteShader
#define glCreateProgram px_glCreateProgram
#define glAttachShader px_glAttachShader
#define glBindAttribLocation px_glBindAttribLocation
#define glLinkProgram px_glLinkProgram
#define glGetProgramiv px_glGetProgramiv
#define glGetProgramInfoLog px_glGetProgramInfoLog
#define glDeleteProgram px_glDeleteProgram
#define glUseProgram px_glUseProgram
#define glGetUniformLocation px_glGetUniformLocation
#define glUniform2f px_glUniform2f
#define glUniform1i px_glUniform1i
#define glActiveTexture px_glActiveTexture
#define glGenVertexArrays px_glGenVertexArrays
#define glBindVertexArray px_glBindVertexArray
#define glGenBuffers px_glGenBuffers
#define glBindBuffer px_glBindBuffer
#define glBufferData px_glBufferData
#define glBufferSubData px_glBufferSubData
#define glTexBuffer px_glTexBuffer
#define glDrawArraysInstanced px_glDrawArraysInstanced
#define glEnableVertexAttribArray px_glEnableVertexAttribArray
#define glVertexAttribPointer px_glVertexAttribPointer

#elif defined(__linux__)

// Unlike Windows, ST's Linux binary links every GL entry point it uses -- including the ones past
// 1.1 -- directly against libGL.so.1 (confirmed: glCreateShader, glGenFramebuffers and friends are
// ordinary NEEDED-library imports in its .dynsym, not resolved through glXGetProcAddress, and the
// binary contains no "glX" strings at all). That is only possible because Mesa's libGL.so exports
// the full core profile as real symbols rather than gating it behind the extension-query API the
// GLX spec technically requires; ST is relying on that Mesa behavior rather than working around
// it. This header does the same: plain prototypes, resolved by the dynamic linker at load time, no
// function-pointer indirection. px_gl_has_shaders() is unconditionally true for the same reason
// px.h's macOS branch is -- if the symbols were not there, the process would have failed to start.
//
// The FBO/renderbuffer entry points are not part of ST's own drawing surface (mac's px_gl_layer.mm
// and win's px_gl.cc both get a ready-made default framebuffer from the platform: a CAOpenGLLayer
// or an HDC-backed HGLRC). GTK3 has no such thing -- its compositor is cairo, not GL -- so the
// Linux backend owns an FBO and blits it into the widget's cairo surface with
// gdk_cairo_draw_from_gl every "draw". ST links exactly this same GL_FRAMEBUFFER/GL_RENDERBUFFER
// surface directly too, which is how that blit target gets built there.

#include <cstdint>

using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLbyte = signed char;
using GLshort = short;
using GLint = int;
using GLsizei = int;
using GLubyte = unsigned char;
using GLushort = unsigned short;
using GLuint = unsigned int;
using GLfloat = float;
using GLclampf = float;
using GLdouble = double;
using GLvoid = void;
using GLchar = char;
using GLsizeiptr = int64_t;
using GLintptr = int64_t;

#define GL_FALSE 0
#define GL_TRUE 1
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_ONE 1
#define GL_EQUAL 0x0202
#define GL_ALWAYS 0x0207
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_STENCIL_TEST 0x0B90
#define GL_FLOAT 0x1406
#define GL_VERSION 0x1F02
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_STREAM_DRAW 0x88E0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_RGBA32F 0x8814
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_RGBA8 0x8058
#define GL_STENCIL_INDEX8 0x8D48

extern "C" {
void glClear(GLbitfield mask);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glClearStencil(GLint value);
void glBlendFunc(GLenum source, GLenum destination);
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glDisable(GLenum cap);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glEnable(GLenum cap);
void glFlush(void);
void glGenTextures(GLsizei, GLuint*);
void glBindTexture(GLenum, GLuint);
const GLubyte* glGetString(GLenum name);
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h);
void glStencilFunc(GLenum function, GLint reference, GLuint mask);
void glStencilMask(GLuint mask);
void glStencilOp(GLenum fail, GLenum depth_fail, GLenum depth_pass);
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);

GLuint glCreateShader(GLenum);
void glShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*);
void glCompileShader(GLuint);
void glGetShaderiv(GLuint, GLenum, GLint*);
void glGetShaderInfoLog(GLuint, GLsizei, GLsizei*, GLchar*);
void glDeleteShader(GLuint);
GLuint glCreateProgram(void);
void glAttachShader(GLuint, GLuint);
void glBindAttribLocation(GLuint, GLuint, const GLchar*);
void glLinkProgram(GLuint);
void glGetProgramiv(GLuint, GLenum, GLint*);
void glGetProgramInfoLog(GLuint, GLsizei, GLsizei*, GLchar*);
void glDeleteProgram(GLuint);
void glUseProgram(GLuint);
GLint glGetUniformLocation(GLuint, const GLchar*);
void glUniform2f(GLint, GLfloat, GLfloat);
void glUniform1i(GLint, GLint);
void glActiveTexture(GLenum);
void glGenVertexArrays(GLsizei, GLuint*);
void glBindVertexArray(GLuint);
void glGenBuffers(GLsizei, GLuint*);
void glBindBuffer(GLenum, GLuint);
void glBufferData(GLenum, GLsizeiptr, const void*, GLenum);
void glBufferSubData(GLenum, GLintptr, GLsizeiptr, const void*);
void glTexBuffer(GLenum, GLenum, GLuint);
void glDrawArraysInstanced(GLenum, GLint, GLsizei, GLsizei);
void glEnableVertexAttribArray(GLuint);
void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);

void glGenFramebuffers(GLsizei, GLuint*);
void glDeleteFramebuffers(GLsizei, const GLuint*);
void glBindFramebuffer(GLenum, GLuint);
void glFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint);
GLenum glCheckFramebufferStatus(GLenum);
void glGenRenderbuffers(GLsizei, GLuint*);
void glDeleteRenderbuffers(GLsizei, const GLuint*);
void glBindRenderbuffer(GLenum, GLuint);
void glRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei);
}

inline bool px_gl_has_shaders() { return true; }

#else
#error "px_gl.h has no backend for this platform"
#endif
