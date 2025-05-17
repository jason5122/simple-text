#pragma once

#include "opengl/gl_enums.h"
#include "opengl/gl_types.h"

namespace opengl {

// 1.0
extern PFNGLCLEARPROC glClear;
extern PFNGLCLEARCOLORPROC glClearColor;
extern PFNGLDEPTHMASKPROC glDepthMask;
extern PFNGLENABLEPROC glEnable;
extern PFNGLGETERRORPROC glGetError;
extern PFNGLGETINTEGERVPROC glGetIntegerv;
extern PFNGLVIEWPORTPROC glViewport;
extern PFNGLPIXELSTOREIPROC glPixelStorei;
extern PFNGLTEXIMAGE2DPROC glTexImage2D;
extern PFNGLTEXPARAMETERIPROC glTexParameteri;
extern PFNGLBLENDFUNCPROC glBlendFunc;
extern PFNGLSCISSORPROC glScissor;
extern PFNGLDISABLEPROC glDisable;
extern PFNGLFLUSHPROC glFlush;

// 1.1
extern PFNGLGENTEXTURESPROC glGenTextures;
extern PFNGLDELETETEXTURESPROC glDeleteTextures;
extern PFNGLBINDTEXTUREPROC glBindTexture;
extern PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;

// 1.3
extern PFNGLACTIVETEXTUREPROC glActiveTexture;

// 1.4
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;

// 1.5
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;

// 2.0
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM1IPROC glUniform1i;

// 3.0
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;

// 3.1
extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;

// 3.3
extern PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;

}  // namespace opengl
