#include "gl.h"

namespace opengl {

// 1.0
PFNGLCLEARPROC glClear = nullptr;
PFNGLCLEARCOLORPROC glClearColor = nullptr;
PFNGLDEPTHMASKPROC glDepthMask = nullptr;
PFNGLENABLEPROC glEnable = nullptr;
PFNGLGETERRORPROC glGetError = nullptr;
PFNGLVIEWPORTPROC glViewport = nullptr;
PFNGLPIXELSTOREIPROC glPixelStorei = nullptr;
PFNGLTEXIMAGE2DPROC glTexImage2D = nullptr;
PFNGLTEXPARAMETERIPROC glTexParameteri = nullptr;
PFNGLBLENDFUNCPROC glBlendFunc = nullptr;
PFNGLSCISSORPROC glScissor = nullptr;
PFNGLDISABLEPROC glDisable = nullptr;

// 1.1
PFNGLGENTEXTURESPROC glGenTextures = nullptr;
PFNGLDELETETEXTURESPROC glDeleteTextures = nullptr;
PFNGLBINDTEXTUREPROC glBindTexture = nullptr;
PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D = nullptr;

// 1.3
PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

// 1.4
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate = nullptr;

// 1.5
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLBUFFERSUBDATAPROC glBufferSubData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;

// 2.0
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
PFNGLUNIFORM1FPROC glUniform1f = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;

// 3.0
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;
PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer = nullptr;

// 3.1
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = nullptr;

// 3.3
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = nullptr;

}
