#pragma once

#include "build/build_config.h"
#include "third_party/khronos/KHR/khrplatform.h"
#include <stdint.h>

// DEV: The section with OpenGL function declarations (lines with `extern`) are at the bottom.

namespace gl {

// Types.
typedef void GLvoid;
typedef char GLchar;
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef khronos_int8_t GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef khronos_uint8_t GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef khronos_float_t GLfloat;
typedef khronos_float_t GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef khronos_int32_t GLfixed;
typedef khronos_intptr_t GLintptr;
typedef khronos_ssize_t GLsizeiptr;
typedef unsigned short GLhalf;
typedef khronos_int64_t GLint64;
typedef khronos_uint64_t GLuint64;
typedef struct __GLsync* GLsync;

// Functions.
#ifndef INTERNAL_GL_APIENTRY
#if BUILDFLAG(IS_WIN)
#define INTERNAL_GL_APIENTRY __stdcall
#else
#define INTERNAL_GL_APIENTRY
#endif
#endif

typedef void(INTERNAL_GL_APIENTRY* GLDEBUGPROC)(GLenum source,
                                                GLenum type,
                                                GLuint id,
                                                GLenum severity,
                                                GLsizei length,
                                                const GLchar* message,
                                                const void* userParam);
typedef void(INTERNAL_GL_APIENTRY* GLDEBUGPROCARB)(GLenum source,
                                                   GLenum type,
                                                   GLuint id,
                                                   GLenum severity,
                                                   GLsizei length,
                                                   const GLchar* message,
                                                   const void* userParam);
typedef void(INTERNAL_GL_APIENTRY* GLDEBUGPROCAMD)(GLuint id,
                                                   GLenum category,
                                                   GLenum severity,
                                                   GLsizei length,
                                                   const GLchar* message,
                                                   void* userParam);

// 1.0
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDFUNCPROC)(GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARPROC)(GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARDEPTHPROC)(GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARSTENCILPROC)(GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOLORMASKPROC)(GLboolean, GLboolean, GLboolean, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCULLFACEPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEPTHFUNCPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEPTHMASKPROC)(GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEPTHRANGEPROC)(GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISABLEPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWBUFFERPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENABLEPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFINISHPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLFLUSHPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRONTFACEPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETBOOLEANVPROC)(GLenum, GLboolean*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETDOUBLEVPROC)(GLenum, GLdouble*);
typedef GLenum(INTERNAL_GL_APIENTRY* PFNGLGETERRORPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETFLOATVPROC)(GLenum, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTEGERVPROC)(GLenum, GLint*);
typedef const GLubyte*(INTERNAL_GL_APIENTRY* PFNGLGETSTRINGPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXIMAGEPROC)(GLenum, GLint, GLenum, GLenum, GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXLEVELPARAMETERFVPROC)(GLenum,
                                                                    GLint,
                                                                    GLenum,
                                                                    GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXLEVELPARAMETERIVPROC)(GLenum, GLint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXPARAMETERFVPROC)(GLenum, GLenum, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXPARAMETERIVPROC)(GLenum, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLHINTPROC)(GLenum, GLenum);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISENABLEDPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLLINEWIDTHPROC)(GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLLOGICOPPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPIXELSTOREFPROC)(GLenum, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPIXELSTOREIPROC)(GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOINTSIZEPROC)(GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOLYGONMODEPROC)(GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLREADBUFFERPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLREADPIXELSPROC)(
    GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSCISSORPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSTENCILFUNCPROC)(GLenum, GLint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSTENCILMASKPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSTENCILOPPROC)(GLenum, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXIMAGE1DPROC)(
    GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXIMAGE2DPROC)(
    GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXPARAMETERFPROC)(GLenum, GLenum, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXPARAMETERFVPROC)(GLenum, GLenum, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXPARAMETERIVPROC)(GLenum, GLenum, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);

// 1.1
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDTEXTUREPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXIMAGE1DPROC)(
    GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXIMAGE2DPROC)(
    GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXSUBIMAGE1DPROC)(
    GLenum, GLint, GLint, GLint, GLint, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXSUBIMAGE2DPROC)(
    GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWARRAYSPROC)(GLenum, GLint, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSPROC)(GLenum, GLsizei, GLenum, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENTEXTURESPROC)(GLsizei, GLuint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISTEXTUREPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOLYGONOFFSETPROC)(GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSUBIMAGE1DPROC)(
    GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSUBIMAGE2DPROC)(
    GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);

// 1.2
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDEQUATIONPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXSUBIMAGE3DPROC)(
    GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWRANGEELEMENTSPROC)(
    GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXIMAGE3DPROC)(
    GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSUBIMAGE3DPROC)(
    GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);

// 1.2 Extensions
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEFENCESNVPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENFENCESNVPROC)(GLsizei, GLuint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISFENCENVPROC)(GLuint);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLTESTFENCENVPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETFENCEIVNVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFINISHFENCENVPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSETFENCENVPROC)(GLuint, GLenum);

// 1.3
typedef void(INTERNAL_GL_APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXIMAGE1DPROC)(
    GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXIMAGE2DPROC)(
    GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXIMAGE3DPROC)(
    GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)(
    GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)(
    GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)(
    GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETCOMPRESSEDTEXIMAGEPROC)(GLenum, GLint, GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLECOVERAGEPROC)(GLfloat, GLboolean);

// 1.4
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDFUNCSEPARATEPROC)(GLenum, GLenum, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMULTIDRAWARRAYSPROC)(GLenum,
                                                             const GLint*,
                                                             const GLsizei*,
                                                             GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMULTIDRAWELEMENTSPROC)(
    GLenum, const GLsizei*, GLenum, const GLvoid* const*, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOINTPARAMETERFPROC)(GLenum, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOINTPARAMETERFVPROC)(GLenum, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOINTPARAMETERIPROC)(GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOINTPARAMETERIVPROC)(GLenum, const GLint*);

// 1.5
typedef void(INTERNAL_GL_APIENTRY* PFNGLBEGINQUERYPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBUFFERDATAPROC)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBUFFERSUBDATAPROC)(GLenum,
                                                           GLintptr,
                                                           GLsizeiptr,
                                                           const GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEQUERIESPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENDQUERYPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENBUFFERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENQUERIESPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETBUFFERPARAMETERIVPROC)(GLenum, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETBUFFERPOINTERVPROC)(GLenum, GLenum, GLvoid**);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETBUFFERSUBDATAPROC)(GLenum,
                                                              GLintptr,
                                                              GLsizeiptr,
                                                              GLvoid*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYOBJECTIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYOBJECTUIVPROC)(GLuint, GLenum, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYIVPROC)(GLenum, GLenum, GLint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISBUFFERPROC)(GLuint);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISQUERYPROC)(GLuint);
typedef void*(INTERNAL_GL_APIENTRY* PFNGLMAPBUFFERPROC)(GLenum, GLenum);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLUNMAPBUFFERPROC)(GLenum);

// 2.0
typedef void(INTERNAL_GL_APIENTRY* PFNGLATTACHSHADERPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDATTRIBLOCATIONPROC)(GLuint, GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDEQUATIONSEPARATEPROC)(GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint);
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLCREATEPROGRAMPROC)();
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLCREATESHADERPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETESHADERPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDETACHSHADERPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWBUFFERSPROC)(GLsizei, const GLenum*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEATTRIBPROC)(
    GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEUNIFORMPROC)(
    GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETATTACHEDSHADERSPROC)(GLuint,
                                                                GLsizei,
                                                                GLsizei*,
                                                                GLuint*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETATTRIBLOCATIONPROC)(GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSHADERSOURCEPROC)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSHADERIVPROC)(GLuint, GLenum, GLint*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMFVPROC)(GLuint, GLint, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMIVPROC)(GLuint, GLint, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBPOINTERVPROC)(GLuint, GLenum, GLvoid**);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBDVPROC)(GLuint, GLenum, GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBFVPROC)(GLuint, GLenum, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBIVPROC)(GLuint, GLenum, GLint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISPROGRAMPROC)(GLuint);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISSHADERPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint,
                                                          GLsizei,
                                                          const GLchar* const*,
                                                          const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSTENCILFUNCSEPARATEPROC)(GLenum, GLenum, GLint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSTENCILMASKSEPARATEPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSTENCILOPSEPARATEPROC)(GLenum, GLenum, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1FPROC)(GLint, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1FVPROC)(GLint, GLsizei, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1IPROC)(GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1IVPROC)(GLint, GLsizei, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2FPROC)(GLint, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2FVPROC)(GLint, GLsizei, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2IPROC)(GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2IVPROC)(GLint, GLsizei, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3FPROC)(GLint, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3FVPROC)(GLint, GLsizei, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3IPROC)(GLint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3IVPROC)(GLint, GLsizei, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4FPROC)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4FVPROC)(GLint, GLsizei, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4IPROC)(GLint, GLint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4IVPROC)(GLint, GLsizei, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX2FVPROC)(GLint,
                                                              GLsizei,
                                                              GLboolean,
                                                              const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX3FVPROC)(GLint,
                                                              GLsizei,
                                                              GLboolean,
                                                              const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX4FVPROC)(GLint,
                                                              GLsizei,
                                                              GLboolean,
                                                              const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVALIDATEPROGRAMPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB1DPROC)(GLuint, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB1DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB1FPROC)(GLuint, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB1FVPROC)(GLuint, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB1SPROC)(GLuint, GLshort);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB1SVPROC)(GLuint, const GLshort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB2DPROC)(GLuint, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB2DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB2FPROC)(GLuint, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB2FVPROC)(GLuint, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB2SPROC)(GLuint, GLshort, GLshort);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB2SVPROC)(GLuint, const GLshort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB3DPROC)(GLuint, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB3DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB3FPROC)(GLuint, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB3FVPROC)(GLuint, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB3SPROC)(GLuint, GLshort, GLshort, GLshort);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB3SVPROC)(GLuint, const GLshort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NBVPROC)(GLuint, const GLbyte*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NIVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NSVPROC)(GLuint, const GLshort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NUBPROC)(
    GLuint, GLubyte, GLubyte, GLubyte, GLubyte);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NUBVPROC)(GLuint, const GLubyte*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NUIVPROC)(GLuint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4NUSVPROC)(GLuint, const GLushort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4BVPROC)(GLuint, const GLbyte*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4DPROC)(
    GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4FPROC)(
    GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4FVPROC)(GLuint, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4IVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4SPROC)(
    GLuint, GLshort, GLshort, GLshort, GLshort);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4SVPROC)(GLuint, const GLshort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4UBVPROC)(GLuint, const GLubyte*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4UIVPROC)(GLuint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIB4USVPROC)(GLuint, const GLushort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBPOINTERPROC)(
    GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);

// 2.1
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX2X3FVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX2X4FVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX3X2FVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX3X4FVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX4X2FVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX4X3FVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLfloat*);

// 3.0
typedef void(INTERNAL_GL_APIENTRY* PFNGLBEGINCONDITIONALRENDERPROC)(GLuint, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBEGINTRANSFORMFEEDBACKPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDBUFFERBASEPROC)(GLenum, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDBUFFERRANGEPROC)(
    GLenum, GLuint, GLuint, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDFRAGDATALOCATIONPROC)(GLuint, GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLITFRAMEBUFFERPROC)(
    GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef GLenum(INTERNAL_GL_APIENTRY* PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLAMPCOLORPROC)(GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARBUFFERFIPROC)(GLenum, GLint, GLfloat, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARBUFFERFVPROC)(GLenum, GLint, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARBUFFERIVPROC)(GLenum, GLint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARBUFFERUIVPROC)(GLenum, GLint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOLORMASKIPROC)(
    GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISABLEIPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENABLEIPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENDCONDITIONALRENDERPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLENDTRANSFORMFEEDBACKPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLFLUSHMAPPEDBUFFERRANGEPROC)(GLenum, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum,
                                                                     GLenum,
                                                                     GLenum,
                                                                     GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE1DPROC)(
    GLenum, GLenum, GLenum, GLuint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DPROC)(
    GLenum, GLenum, GLenum, GLuint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE3DPROC)(
    GLenum, GLenum, GLenum, GLuint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURELAYERPROC)(
    GLenum, GLenum, GLuint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENERATEMIPMAPPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETBOOLEANI_VPROC)(GLenum, GLuint, GLboolean*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETFRAGDATALOCATIONPROC)(GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)(GLenum,
                                                                                 GLenum,
                                                                                 GLenum,
                                                                                 GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTEGERI_VPROC)(GLenum, GLuint, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETRENDERBUFFERPARAMETERIVPROC)(GLenum, GLenum, GLint*);
typedef const GLubyte*(INTERNAL_GL_APIENTRY* PFNGLGETSTRINGIPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXPARAMETERIIVPROC)(GLenum, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXPARAMETERIUIVPROC)(GLenum, GLenum, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)(
    GLuint, GLuint, GLsizei, GLsizei*, GLsizei*, GLenum*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMUIVPROC)(GLuint, GLint, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBIIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBIUIVPROC)(GLuint, GLenum, GLuint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISENABLEDIPROC)(GLenum, GLuint);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISFRAMEBUFFERPROC)(GLuint);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISRENDERBUFFERPROC)(GLuint);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISVERTEXARRAYPROC)(GLuint);
typedef void*(INTERNAL_GL_APIENTRY* PFNGLMAPBUFFERRANGEPROC)(GLenum,
                                                             GLintptr,
                                                             GLsizeiptr,
                                                             GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXPARAMETERIIVPROC)(GLenum, GLenum, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXPARAMETERIUIVPROC)(GLenum, GLenum, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTRANSFORMFEEDBACKVARYINGSPROC)(GLuint,
                                                                       GLsizei,
                                                                       const GLchar* const*,
                                                                       GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1UIPROC)(GLint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1UIVPROC)(GLint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2UIPROC)(GLint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2UIVPROC)(GLint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3UIPROC)(GLint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3UIVPROC)(GLint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4UIPROC)(GLint, GLuint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4UIVPROC)(GLint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI1IPROC)(GLuint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI1IVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI1UIPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI1UIVPROC)(GLuint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI2IPROC)(GLuint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI2IVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI2UIPROC)(GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI2UIVPROC)(GLuint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI3IPROC)(GLuint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI3IVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI3UIPROC)(GLuint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI3UIVPROC)(GLuint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4BVPROC)(GLuint, const GLbyte*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4IPROC)(GLuint, GLint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4IVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4SVPROC)(GLuint, const GLshort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4UBVPROC)(GLuint, const GLubyte*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4UIPROC)(
    GLuint, GLuint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4UIVPROC)(GLuint, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBI4USVPROC)(GLuint, const GLushort*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBIPOINTERPROC)(
    GLuint, GLint, GLenum, GLsizei, const GLvoid*);

// 3.1
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYBUFFERSUBDATAPROC)(
    GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDPROC)(
    GLenum, GLsizei, GLenum, const GLvoid*, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)(
    GLuint, GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEUNIFORMBLOCKIVPROC)(GLuint,
                                                                     GLuint,
                                                                     GLenum,
                                                                     GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEUNIFORMNAMEPROC)(
    GLuint, GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEUNIFORMSIVPROC)(
    GLuint, GLsizei, const GLuint*, GLenum, GLint*);
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMBLOCKINDEXPROC)(GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMINDICESPROC)(GLuint,
                                                               GLsizei,
                                                               const GLchar* const*,
                                                               GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPRIMITIVERESTARTINDEXPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXBUFFERPROC)(GLenum, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMBLOCKBINDINGPROC)(GLuint, GLuint, GLuint);

// 3.2
typedef GLenum(INTERNAL_GL_APIENTRY* PFNGLCLIENTWAITSYNCPROC)(GLsync, GLbitfield, GLuint64);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETESYNCPROC)(GLsync);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSBASEVERTEXPROC)(
    GLenum, GLsizei, GLenum, const GLvoid*, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)(
    GLenum, GLsizei, GLenum, const GLvoid*, GLsizei, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)(
    GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid*, GLint);
typedef GLsync(INTERNAL_GL_APIENTRY* PFNGLFENCESYNCPROC)(GLenum, GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREPROC)(GLenum, GLenum, GLuint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETBUFFERPARAMETERI64VPROC)(GLenum, GLenum, GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTEGER64I_VPROC)(GLenum, GLuint, GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTEGER64VPROC)(GLenum, GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETMULTISAMPLEFVPROC)(GLenum, GLuint, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSYNCIVPROC)(GLsync, GLenum, GLsizei, GLsizei*, GLint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISSYNCPROC)(GLsync);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)(
    GLenum, const GLsizei*, GLenum, const GLvoid* const*, GLsizei, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROVOKINGVERTEXPROC)(GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLEMASKIPROC)(GLuint, GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXIMAGE2DMULTISAMPLEPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXIMAGE3DMULTISAMPLEPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLWAITSYNCPROC)(GLsync, GLbitfield, GLuint64);

// 3.3
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)(GLuint,
                                                                         GLuint,
                                                                         GLuint,
                                                                         const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDSAMPLERPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETESAMPLERSPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENSAMPLERSPROC)(GLsizei, GLuint*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETFRAGDATAINDEXPROC)(GLuint, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYOBJECTI64VPROC)(GLuint, GLenum, GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYOBJECTUI64VPROC)(GLuint, GLenum, GLuint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSAMPLERPARAMETERIIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSAMPLERPARAMETERIUIVPROC)(GLuint, GLenum, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSAMPLERPARAMETERFVPROC)(GLuint, GLenum, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSAMPLERPARAMETERIVPROC)(GLuint, GLenum, GLint*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISSAMPLERPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLQUERYCOUNTERPROC)(GLuint, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLERPARAMETERIIVPROC)(GLuint, GLenum, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLERPARAMETERIUIVPROC)(GLuint, GLenum, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLERPARAMETERFPROC)(GLuint, GLenum, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLERPARAMETERFVPROC)(GLuint, GLenum, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLERPARAMETERIPROC)(GLuint, GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSAMPLERPARAMETERIVPROC)(GLuint, GLenum, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBDIVISORPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP1UIPROC)(GLuint, GLenum, GLboolean, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP1UIVPROC)(GLuint,
                                                               GLenum,
                                                               GLboolean,
                                                               const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP2UIPROC)(GLuint, GLenum, GLboolean, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP2UIVPROC)(GLuint,
                                                               GLenum,
                                                               GLboolean,
                                                               const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP3UIPROC)(GLuint, GLenum, GLboolean, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP3UIVPROC)(GLuint,
                                                               GLenum,
                                                               GLboolean,
                                                               const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP4UIPROC)(GLuint, GLenum, GLboolean, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBP4UIVPROC)(GLuint,
                                                               GLenum,
                                                               GLboolean,
                                                               const GLuint*);

// 4.0
typedef void(INTERNAL_GL_APIENTRY* PFNGLBEGINQUERYINDEXEDPROC)(GLenum, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDTRANSFORMFEEDBACKPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDEQUATIONSEPARATEIPROC)(GLuint, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDEQUATIONIPROC)(GLuint, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDFUNCSEPARATEIPROC)(
    GLuint, GLenum, GLenum, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDFUNCIPROC)(GLuint, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETETRANSFORMFEEDBACKSPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWARRAYSINDIRECTPROC)(GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSINDIRECTPROC)(GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWTRANSFORMFEEDBACKPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)(GLenum, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENDQUERYINDEXEDPROC)(GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENTRANSFORMFEEDBACKSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVESUBROUTINENAMEPROC)(
    GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)(
    GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)(
    GLuint, GLenum, GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMSTAGEIVPROC)(GLuint, GLenum, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYINDEXEDIVPROC)(GLenum, GLuint, GLenum, GLint*);
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLGETSUBROUTINEINDEXPROC)(GLuint, GLenum, const GLchar*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)(GLuint,
                                                                           GLenum,
                                                                           const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMSUBROUTINEUIVPROC)(GLenum, GLint, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNIFORMDVPROC)(GLuint, GLint, GLdouble*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISTRANSFORMFEEDBACKPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMINSAMPLESHADINGPROC)(GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPATCHPARAMETERFVPROC)(GLenum, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPATCHPARAMETERIPROC)(GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPAUSETRANSFORMFEEDBACKPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLRESUMETRANSFORMFEEDBACKPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1DPROC)(GLint, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM1DVPROC)(GLint, GLsizei, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2DPROC)(GLint, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM2DVPROC)(GLint, GLsizei, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3DPROC)(GLint, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM3DVPROC)(GLint, GLsizei, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4DPROC)(
    GLint, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORM4DVPROC)(GLint, GLsizei, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX2DVPROC)(GLint,
                                                              GLsizei,
                                                              GLboolean,
                                                              const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX2X3DVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX2X4DVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX3DVPROC)(GLint,
                                                              GLsizei,
                                                              GLboolean,
                                                              const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX3X2DVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX3X4DVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX4DVPROC)(GLint,
                                                              GLsizei,
                                                              GLboolean,
                                                              const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX4X2DVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMMATRIX4X3DVPROC)(GLint,
                                                                GLsizei,
                                                                GLboolean,
                                                                const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUNIFORMSUBROUTINESUIVPROC)(GLenum, GLsizei, const GLuint*);

// 4.1
typedef void(INTERNAL_GL_APIENTRY* PFNGLACTIVESHADERPROGRAMPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDPROGRAMPIPELINEPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARDEPTHFPROC)(GLfloat);
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLCREATESHADERPROGRAMVPROC)(GLenum,
                                                                    GLsizei,
                                                                    const GLchar* const*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEPROGRAMPIPELINESPROC)(GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEPTHRANGEARRAYVPROC)(GLuint, GLsizei, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEPTHRANGEINDEXEDPROC)(GLuint, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEPTHRANGEFPROC)(GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENPROGRAMPIPELINESPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETDOUBLEI_VPROC)(GLenum, GLuint, GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETFLOATI_VPROC)(GLenum, GLuint, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMBINARYPROC)(
    GLuint, GLsizei, GLsizei*, GLenum*, void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMPIPELINEINFOLOGPROC)(GLuint,
                                                                       GLsizei,
                                                                       GLsizei*,
                                                                       GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMPIPELINEIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSHADERPRECISIONFORMATPROC)(GLenum,
                                                                      GLenum,
                                                                      GLint*,
                                                                      GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXATTRIBLDVPROC)(GLuint, GLenum, GLdouble*);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISPROGRAMPIPELINEPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMBINARYPROC)(GLuint, GLenum, const void*, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMPARAMETERIPROC)(GLuint, GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1DPROC)(GLuint, GLint, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1DVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1FPROC)(GLuint, GLint, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1FVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1IPROC)(GLuint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1IVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1UIPROC)(GLuint, GLint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM1UIVPROC)(GLuint,
                                                                GLint,
                                                                GLsizei,
                                                                const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2DPROC)(GLuint, GLint, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2DVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2FPROC)(GLuint, GLint, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2FVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2IPROC)(GLuint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2IVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2UIPROC)(GLuint, GLint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM2UIVPROC)(GLuint,
                                                                GLint,
                                                                GLsizei,
                                                                const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3DPROC)(
    GLuint, GLint, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3DVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3FPROC)(
    GLuint, GLint, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3FVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3IPROC)(GLuint, GLint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3IVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3UIPROC)(
    GLuint, GLint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM3UIVPROC)(GLuint,
                                                                GLint,
                                                                GLsizei,
                                                                const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4DPROC)(
    GLuint, GLint, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4DVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4FPROC)(
    GLuint, GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4FVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4IPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4IVPROC)(GLuint,
                                                               GLint,
                                                               GLsizei,
                                                               const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4UIPROC)(
    GLuint, GLint, GLuint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORM4UIVPROC)(GLuint,
                                                                GLint,
                                                                GLsizei,
                                                                const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)(
    GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLRELEASESHADERCOMPILERPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLSCISSORARRAYVPROC)(GLuint, GLsizei, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSCISSORINDEXEDPROC)(
    GLuint, GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSCISSORINDEXEDVPROC)(GLuint, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSHADERBINARYPROC)(
    GLsizei, const GLuint*, GLenum, const void*, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLUSEPROGRAMSTAGESPROC)(GLuint, GLbitfield, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVALIDATEPROGRAMPIPELINEPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL1DPROC)(GLuint, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL1DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL2DPROC)(GLuint, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL2DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL3DPROC)(GLuint, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL3DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL4DPROC)(
    GLuint, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBL4DVPROC)(GLuint, const GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBLPOINTERPROC)(
    GLuint, GLint, GLenum, GLsizei, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVIEWPORTARRAYVPROC)(GLuint, GLsizei, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVIEWPORTINDEXEDFPROC)(
    GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVIEWPORTINDEXEDFVPROC)(GLuint, const GLfloat*);

// 4.2
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDIMAGETEXTUREPROC)(
    GLuint, GLuint, GLint, GLboolean, GLint, GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)(
    GLenum, GLint, GLsizei, GLsizei, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)(
    GLenum, GLsizei, GLenum, const void*, GLsizei, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)(
    GLenum, GLsizei, GLenum, const void*, GLsizei, GLint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)(GLenum,
                                                                            GLuint,
                                                                            GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)(GLenum,
                                                                                  GLuint,
                                                                                  GLuint,
                                                                                  GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)(GLuint,
                                                                            GLuint,
                                                                            GLenum,
                                                                            GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTERNALFORMATIVPROC)(
    GLenum, GLenum, GLenum, GLsizei, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMEMORYBARRIERPROC)(GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGE1DPROC)(GLenum, GLsizei, GLenum, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGE2DPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGE3DPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);

// 4.3
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDVERTEXBUFFERPROC)(GLuint, GLuint, GLintptr, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARBUFFERDATAPROC)(
    GLenum, GLenum, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARBUFFERSUBDATAPROC)(
    GLenum, GLenum, GLintptr, GLsizeiptr, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYIMAGESUBDATAPROC)(GLuint,
                                                              GLenum,
                                                              GLint,
                                                              GLint,
                                                              GLint,
                                                              GLint,
                                                              GLuint,
                                                              GLenum,
                                                              GLint,
                                                              GLint,
                                                              GLint,
                                                              GLint,
                                                              GLsizei,
                                                              GLsizei,
                                                              GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEBUGMESSAGECONTROLPROC)(
    GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDEBUGMESSAGEINSERTPROC)(
    GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISPATCHCOMPUTEPROC)(GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISPATCHCOMPUTEINDIRECTPROC)(GLintptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERPARAMETERIPROC)(GLenum, GLenum, GLint);
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLGETDEBUGMESSAGELOGPROC)(
    GLuint, GLsizei, GLenum*, GLenum*, GLuint*, GLenum*, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETFRAMEBUFFERPARAMETERIVPROC)(GLenum, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTERNALFORMATI64VPROC)(
    GLenum, GLenum, GLenum, GLsizei, GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPOINTERVPROC)(GLenum, void**);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETOBJECTLABELPROC)(
    GLenum, GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETOBJECTPTRLABELPROC)(const void*,
                                                               GLsizei,
                                                               GLsizei*,
                                                               GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMINTERFACEIVPROC)(GLuint, GLenum, GLenum, GLint*);
typedef GLuint(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMRESOURCEINDEXPROC)(GLuint,
                                                                       GLenum,
                                                                       const GLchar*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMRESOURCELOCATIONPROC)(GLuint,
                                                                         GLenum,
                                                                         const GLchar*);
typedef GLint(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)(GLuint,
                                                                              GLenum,
                                                                              const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMRESOURCENAMEPROC)(
    GLuint, GLenum, GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETPROGRAMRESOURCEIVPROC)(
    GLuint, GLenum, GLuint, GLsizei, const GLenum*, GLsizei, GLsizei*, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATEBUFFERDATAPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATEBUFFERSUBDATAPROC)(GLuint, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATEFRAMEBUFFERPROC)(GLenum, GLsizei, const GLenum*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATESUBFRAMEBUFFERPROC)(
    GLenum, GLsizei, const GLenum*, GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATETEXIMAGEPROC)(GLuint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATETEXSUBIMAGEPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMULTIDRAWARRAYSINDIRECTPROC)(GLenum,
                                                                     const void*,
                                                                     GLsizei,
                                                                     GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMULTIDRAWELEMENTSINDIRECTPROC)(
    GLenum, GLenum, const void*, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLOBJECTLABELPROC)(GLenum, GLuint, GLsizei, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLOBJECTPTRLABELPROC)(const void*, GLsizei, const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOPDEBUGGROUPPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLPUSHDEBUGGROUPPROC)(GLenum,
                                                            GLuint,
                                                            GLsizei,
                                                            const GLchar*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSHADERSTORAGEBLOCKBINDINGPROC)(GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXBUFFERRANGEPROC)(
    GLenum, GLenum, GLuint, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGE2DMULTISAMPLEPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGE3DMULTISAMPLEPROC)(
    GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREVIEWPROC)(
    GLuint, GLenum, GLuint, GLenum, GLuint, GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBBINDINGPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBFORMATPROC)(
    GLuint, GLint, GLenum, GLboolean, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBIFORMATPROC)(GLuint, GLint, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXATTRIBLFORMATPROC)(GLuint, GLint, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXBINDINGDIVISORPROC)(GLuint, GLuint);

// NV_framebuffer_mixed_samples
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOVERAGEMODULATIONNVPROC)(GLenum);

// 4.4
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDBUFFERSBASEPROC)(GLenum,
                                                             GLuint,
                                                             GLsizei,
                                                             const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDBUFFERSRANGEPROC)(
    GLenum, GLuint, GLsizei, const GLuint*, const GLintptr*, const GLsizeiptr*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDIMAGETEXTURESPROC)(GLuint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDSAMPLERSPROC)(GLuint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDTEXTURESPROC)(GLuint, GLsizei, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDVERTEXBUFFERSPROC)(
    GLuint, GLsizei, const GLuint*, const GLintptr*, const GLsizei*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBUFFERSTORAGEPROC)(GLenum,
                                                           GLsizeiptr,
                                                           const void*,
                                                           GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARTEXIMAGEPROC)(
    GLuint, GLint, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARTEXSUBIMAGEPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*);

// 4.5
typedef void(INTERNAL_GL_APIENTRY* PFNGLBINDTEXTUREUNITPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLITNAMEDFRAMEBUFFERPROC)(
    GLuint, GLuint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef GLenum(INTERNAL_GL_APIENTRY* PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)(GLuint, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARNAMEDBUFFERDATAPROC)(
    GLuint, GLenum, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARNAMEDBUFFERSUBDATAPROC)(
    GLuint, GLenum, GLintptr, GLsizeiptr, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARNAMEDFRAMEBUFFERFIPROC)(
    GLuint, GLenum, GLint, GLfloat, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)(GLuint,
                                                                     GLenum,
                                                                     GLint,
                                                                     const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARNAMEDFRAMEBUFFERIVPROC)(GLuint,
                                                                     GLenum,
                                                                     GLint,
                                                                     const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC)(GLuint,
                                                                      GLenum,
                                                                      GLint,
                                                                      const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCLIPCONTROLPROC)(GLenum, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC)(
    GLuint, GLint, GLint, GLsizei, GLenum, GLsizei, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC)(
    GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYNAMEDBUFFERSUBDATAPROC)(
    GLuint, GLuint, GLintptr, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXTURESUBIMAGE1DPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXTURESUBIMAGE2DPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCOPYTEXTURESUBIMAGE3DPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATEBUFFERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATEFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATEPROGRAMPIPELINESPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATEQUERIESPROC)(GLenum, GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATERENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATESAMPLERSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATETEXTURESPROC)(GLenum, GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATETRANSFORMFEEDBACKSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATEVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISABLEVERTEXARRAYATTRIBPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLENABLEVERTEXARRAYATTRIBPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC)(GLuint,
                                                                         GLintptr,
                                                                         GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENERATETEXTUREMIPMAPPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC)(GLuint,
                                                                       GLint,
                                                                       GLsizei,
                                                                       void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, void*);
typedef GLenum(INTERNAL_GL_APIENTRY* PFNGLGETGRAPHICSRESETSTATUSPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERI64VPROC)(GLuint, GLenum, GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDBUFFERPOINTERVPROC)(GLuint, GLenum, void**);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDBUFFERSUBDATAPROC)(GLuint,
                                                                   GLintptr,
                                                                   GLsizeiptr,
                                                                   void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC)(GLuint,
                                                                                      GLenum,
                                                                                      GLenum,
                                                                                      GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC)(GLuint,
                                                                            GLenum,
                                                                            GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC)(GLuint,
                                                                             GLenum,
                                                                             GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYBUFFEROBJECTI64VPROC)(GLuint,
                                                                      GLuint,
                                                                      GLenum,
                                                                      GLintptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYBUFFEROBJECTIVPROC)(GLuint,
                                                                    GLuint,
                                                                    GLenum,
                                                                    GLintptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYBUFFEROBJECTUI64VPROC)(GLuint,
                                                                       GLuint,
                                                                       GLenum,
                                                                       GLintptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETQUERYBUFFEROBJECTUIVPROC)(GLuint,
                                                                     GLuint,
                                                                     GLenum,
                                                                     GLintptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTUREIMAGEPROC)(
    GLuint, GLint, GLenum, GLenum, GLsizei, void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTURELEVELPARAMETERFVPROC)(GLuint,
                                                                        GLint,
                                                                        GLenum,
                                                                        GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTURELEVELPARAMETERIVPROC)(GLuint,
                                                                        GLint,
                                                                        GLenum,
                                                                        GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTUREPARAMETERIIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTUREPARAMETERIUIVPROC)(GLuint, GLenum, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTUREPARAMETERFVPROC)(GLuint, GLenum, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTUREPARAMETERIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTEXTURESUBIMAGEPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, GLsizei, void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTRANSFORMFEEDBACKI64_VPROC)(GLuint,
                                                                       GLenum,
                                                                       GLuint,
                                                                       GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTRANSFORMFEEDBACKI_VPROC)(GLuint,
                                                                     GLenum,
                                                                     GLuint,
                                                                     GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETTRANSFORMFEEDBACKIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXARRAYINDEXED64IVPROC)(GLuint,
                                                                       GLuint,
                                                                       GLenum,
                                                                       GLint64*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXARRAYINDEXEDIVPROC)(GLuint,
                                                                     GLuint,
                                                                     GLenum,
                                                                     GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETVERTEXARRAYIVPROC)(GLuint, GLenum, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNCOMPRESSEDTEXIMAGEPROC)(GLenum, GLint, GLsizei, void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNTEXIMAGEPROC)(
    GLenum, GLint, GLenum, GLenum, GLsizei, void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNUNIFORMDVPROC)(GLuint, GLint, GLsizei, GLdouble*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNUNIFORMFVPROC)(GLuint, GLint, GLsizei, GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNUNIFORMIVPROC)(GLuint, GLint, GLsizei, GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETNUNIFORMUIVPROC)(GLuint, GLint, GLsizei, GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC)(GLuint,
                                                                            GLsizei,
                                                                            const GLenum*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC)(
    GLuint, GLsizei, const GLenum*, GLint, GLint, GLsizei, GLsizei);
typedef void*(INTERNAL_GL_APIENTRY* PFNGLMAPNAMEDBUFFERPROC)(GLuint, GLenum);
typedef void*(INTERNAL_GL_APIENTRY* PFNGLMAPNAMEDBUFFERRANGEPROC)(GLuint,
                                                                  GLintptr,
                                                                  GLsizeiptr,
                                                                  GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMEMORYBARRIERBYREGIONPROC)(GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDBUFFERDATAPROC)(GLuint,
                                                             GLsizeiptr,
                                                             const void*,
                                                             GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDBUFFERSTORAGEPROC)(GLuint,
                                                                GLsizeiptr,
                                                                const void*,
                                                                GLbitfield);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDBUFFERSUBDATAPROC)(GLuint,
                                                                GLintptr,
                                                                GLsizeiptr,
                                                                const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC)(GLuint, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)(GLuint,
                                                                         GLsizei,
                                                                         const GLenum*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC)(GLuint, GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC)(GLuint, GLenum);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC)(GLuint,
                                                                          GLenum,
                                                                          GLenum,
                                                                          GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)(GLuint,
                                                                     GLenum,
                                                                     GLuint,
                                                                     GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)(
    GLuint, GLenum, GLuint, GLint, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDRENDERBUFFERSTORAGEPROC)(GLuint,
                                                                      GLenum,
                                                                      GLsizei,
                                                                      GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC)(
    GLuint, GLsizei, GLenum, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLREADNPIXELSPROC)(
    GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREBARRIERPROC)();
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREBUFFERPROC)(GLuint, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREBUFFERRANGEPROC)(
    GLuint, GLenum, GLuint, GLintptr, GLsizeiptr);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREPARAMETERIIVPROC)(GLuint, GLenum, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREPARAMETERIUIVPROC)(GLuint, GLenum, const GLuint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREPARAMETERFPROC)(GLuint, GLenum, GLfloat);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREPARAMETERFVPROC)(GLuint, GLenum, const GLfloat*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREPARAMETERIPROC)(GLuint, GLenum, GLint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTUREPARAMETERIVPROC)(GLuint, GLenum, const GLint*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGE1DPROC)(GLuint, GLsizei, GLenum, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGE2DPROC)(
    GLuint, GLsizei, GLenum, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC)(
    GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGE3DPROC)(
    GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC)(
    GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLsizei, GLboolean);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESUBIMAGE1DPROC)(
    GLuint, GLint, GLint, GLsizei, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESUBIMAGE2DPROC)(
    GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESUBIMAGE3DPROC)(
    GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC)(GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC)(
    GLuint, GLuint, GLuint, GLintptr, GLsizeiptr);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLUNMAPNAMEDBUFFERPROC)(GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYATTRIBBINDINGPROC)(GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYATTRIBFORMATPROC)(
    GLuint, GLuint, GLint, GLenum, GLboolean, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYATTRIBIFORMATPROC)(
    GLuint, GLuint, GLint, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYATTRIBLFORMATPROC)(
    GLuint, GLuint, GLint, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYBINDINGDIVISORPROC)(GLuint, GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYELEMENTBUFFERPROC)(GLuint, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYVERTEXBUFFERPROC)(
    GLuint, GLuint, GLuint, GLintptr, GLsizei);
typedef void(INTERNAL_GL_APIENTRY* PFNGLVERTEXARRAYVERTEXBUFFERSPROC)(
    GLuint, GLuint, GLsizei, const GLuint*, const GLintptr*, const GLsizei*);

// GL_EXT_discard_framebuffer
typedef void(INTERNAL_GL_APIENTRY* PFNGLDISCARDFRAMEBUFFEREXTPROC)(GLenum target,
                                                                   GLsizei numAttachments,
                                                                   const GLenum* attachments);

// GL_OES_EGL_image
typedef void* GLeglImageOES;
typedef void(INTERNAL_GL_APIENTRY* PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target,
                                                                        GLeglImageOES image);
typedef void(INTERNAL_GL_APIENTRY* PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC)(
    GLenum target, GLeglImageOES image);

// ES 3.2
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLENDBARRIERPROC)(void);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPRIMITIVEBOUNDINGBOXPROC)(GLfloat minX,
                                                                  GLfloat minY,
                                                                  GLfloat minZ,
                                                                  GLfloat minW,
                                                                  GLfloat maxX,
                                                                  GLfloat maxY,
                                                                  GLfloat maxZ,
                                                                  GLfloat maxW);

// GL_NV_internalformat_sample_query
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETINTERNALFORMATSAMPLEIVNVPROC)(GLenum target,
                                                                         GLenum internalformat,
                                                                         GLsizei samples,
                                                                         GLenum pname,
                                                                         GLsizei bufSize,
                                                                         GLint* params);

// GL_OVR_multiview2
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(GLenum target,
                                                                            GLenum attachment,
                                                                            GLuint texture,
                                                                            GLint level,
                                                                            GLint baseViewIndex,
                                                                            GLsizei numViews);
// EXT_debug_marker
typedef void(INTERNAL_GL_APIENTRY* PFNGLINSERTEVENTMARKEREXTPROC)(GLsizei length,
                                                                  const GLchar* marker);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPUSHGROUPMARKEREXTPROC)(GLsizei length,
                                                                const GLchar* marker);
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOPGROUPMARKEREXTPROC)(void);

// KHR_parallel_shader_compile
typedef void(INTERNAL_GL_APIENTRY* PFNGLMAXSHADERCOMPILERTHREADSKHRPROC)(GLuint count);

// ARB_parallel_shader_compile
typedef void(INTERNAL_GL_APIENTRY* PFNGLMAXSHADERCOMPILERTHREADSARBPROC)(GLuint count);

// GL_EXT_memory_object
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNSIGNEDBYTEVEXTPROC)(GLenum pname, GLubyte* data);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETUNSIGNEDBYTEI_VEXTPROC)(GLenum target,
                                                                   GLuint index,
                                                                   GLubyte* data);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETEMEMORYOBJECTSEXTPROC)(GLsizei n,
                                                                    const GLuint* memoryObjects);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISMEMORYOBJECTEXTPROC)(GLuint memoryObject);
typedef void(INTERNAL_GL_APIENTRY* PFNGLCREATEMEMORYOBJECTSEXTPROC)(GLsizei n,
                                                                    GLuint* memoryObjects);
typedef void(INTERNAL_GL_APIENTRY* PFNGLMEMORYOBJECTPARAMETERIVEXTPROC)(GLuint memoryObject,
                                                                        GLenum pname,
                                                                        const GLint* params);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETMEMORYOBJECTPARAMETERIVEXTPROC)(GLuint memoryObject,
                                                                           GLenum pname,
                                                                           GLint* params);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGEMEM2DEXTPROC)(GLenum target,
                                                                GLsizei levels,
                                                                GLenum internalFormat,
                                                                GLsizei width,
                                                                GLsizei height,
                                                                GLuint memory,
                                                                GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGEMEM2DMULTISAMPLEEXTPROC)(
    GLenum target,
    GLsizei samples,
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLboolean fixedSampleLocations,
    GLuint memory,
    GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGEMEM3DEXTPROC)(GLenum target,
                                                                GLsizei levels,
                                                                GLenum internalFormat,
                                                                GLsizei width,
                                                                GLsizei height,
                                                                GLsizei depth,
                                                                GLuint memory,
                                                                GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGEMEM3DMULTISAMPLEEXTPROC)(
    GLenum target,
    GLsizei samples,
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLboolean fixedSampleLocations,
    GLuint memory,
    GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLBUFFERSTORAGEMEMEXTPROC)(GLenum target,
                                                                 GLsizeiptr size,
                                                                 GLuint memory,
                                                                 GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGEMEM2DEXTPROC)(GLuint texture,
                                                                    GLsizei levels,
                                                                    GLenum internalFormat,
                                                                    GLsizei width,
                                                                    GLsizei height,
                                                                    GLuint memory,
                                                                    GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGEMEM2DMULTISAMPLEEXTPROC)(
    GLuint texture,
    GLsizei samples,
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLboolean fixedSampleLocations,
    GLuint memory,
    GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGEMEM3DEXTPROC)(GLuint texture,
                                                                    GLsizei levels,
                                                                    GLenum internalFormat,
                                                                    GLsizei width,
                                                                    GLsizei height,
                                                                    GLsizei depth,
                                                                    GLuint memory,
                                                                    GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGEMEM3DMULTISAMPLEEXTPROC)(
    GLuint texture,
    GLsizei samples,
    GLenum internalFormat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLboolean fixedSampleLocations,
    GLuint memory,
    GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLNAMEDBUFFERSTORAGEMEMEXTPROC)(GLuint buffer,
                                                                      GLsizeiptr size,
                                                                      GLuint memory,
                                                                      GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXSTORAGEMEM1DEXTPROC)(GLenum target,
                                                                GLsizei levels,
                                                                GLenum internalFormat,
                                                                GLsizei width,
                                                                GLuint memory,
                                                                GLuint64 offset);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXTURESTORAGEMEM1DEXTPROC)(GLuint texture,
                                                                    GLsizei levels,
                                                                    GLenum internalFormat,
                                                                    GLsizei width,
                                                                    GLuint memory,
                                                                    GLuint64 offset);

// GL_EXT_semaphore
typedef void(INTERNAL_GL_APIENTRY* PFNGLGENSEMAPHORESEXTPROC)(GLsizei n, GLuint* semaphores);
typedef void(INTERNAL_GL_APIENTRY* PFNGLDELETESEMAPHORESEXTPROC)(GLsizei n,
                                                                 const GLuint* semaphores);
typedef GLboolean(INTERNAL_GL_APIENTRY* PFNGLISSEMAPHOREEXTPROC)(GLuint semaphore);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSEMAPHOREPARAMETERUI64VEXTPROC)(GLuint semaphore,
                                                                        GLenum pname,
                                                                        const GLuint64* params);
typedef void(INTERNAL_GL_APIENTRY* PFNGLGETSEMAPHOREPARAMETERUI64VEXTPROC)(GLuint semaphore,
                                                                           GLenum pname,
                                                                           GLuint64* params);
typedef void(INTERNAL_GL_APIENTRY* PFNGLWAITSEMAPHOREEXTPROC)(GLuint semaphore,
                                                              GLuint numBufferBarriers,
                                                              const GLuint* buffers,
                                                              GLuint numTextureBarriers,
                                                              const GLuint* textures,
                                                              const GLenum* srcLayouts);
typedef void(INTERNAL_GL_APIENTRY* PFNGLSIGNALSEMAPHOREEXTPROC)(GLuint semaphore,
                                                                GLuint numBufferBarriers,
                                                                const GLuint* buffers,
                                                                GLuint numTextureBarriers,
                                                                const GLuint* textures,
                                                                const GLenum* dstLayouts);

// GL_EXT_memory_object_fd
typedef void(INTERNAL_GL_APIENTRY* PFNGLIMPORTMEMORYFDEXTPROC)(GLuint memory,
                                                               GLuint64 size,
                                                               GLenum handleType,
                                                               GLint fd);

// GL_EXT_semaphore_fd
typedef void(INTERNAL_GL_APIENTRY* PFNGLIMPORTSEMAPHOREFDEXTPROC)(GLuint semaphore,
                                                                  GLenum handleType,
                                                                  GLint fd);

// GL_EXT_memory_object_win32
typedef void(INTERNAL_GL_APIENTRY* PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC)(GLuint memory,
                                                                        GLuint64 size,
                                                                        GLenum handleType,
                                                                        void* handle);
typedef void(INTERNAL_GL_APIENTRY* PFNGLIMPORTMEMORYWIN32NAMEEXTPROC)(GLuint memory,
                                                                      GLuint64 size,
                                                                      GLenum handleType,
                                                                      const void* name);

// GL_EXT_semaphore_win32
typedef void(INTERNAL_GL_APIENTRY* PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC)(GLuint semaphore,
                                                                           GLenum handleType,
                                                                           void* handle);
typedef void(INTERNAL_GL_APIENTRY* PFNGLIMPORTSEMAPHOREWIN32NAMEEXTPROC)(GLuint semaphore,
                                                                         GLenum handleType,
                                                                         const void* name);

// GL_OES_texture_buffer
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXBUFFEROESPROC)(GLenum, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXBUFFERRANGEOESPROC)(
    GLenum, GLenum, GLuint, GLintptr, GLsizeiptr);

// GL_EXT_texture_buffer
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXBUFFEREXTPROC)(GLenum, GLenum, GLuint);
typedef void(INTERNAL_GL_APIENTRY* PFNGLTEXBUFFERRANGEEXTPROC)(
    GLenum, GLenum, GLuint, GLintptr, GLsizeiptr);

// GL_EXT_framebuffer_blit
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLITFRAMEBUFFEREXTPROC)(GLint srcX0,
                                                                GLint srcY0,
                                                                GLint srcX1,
                                                                GLint srcY1,
                                                                GLint dstX0,
                                                                GLint dstY0,
                                                                GLint dstX1,
                                                                GLint dstY1,
                                                                GLbitfield mask,
                                                                GLenum filter);

// GL_NV_framebuffer_blit
typedef void(INTERNAL_GL_APIENTRY* PFNGLBLITFRAMEBUFFERNVPROC)(GLint srcX0,
                                                               GLint srcY0,
                                                               GLint srcX1,
                                                               GLint srcY1,
                                                               GLint dstX0,
                                                               GLint dstY0,
                                                               GLint dstX1,
                                                               GLint dstY1,
                                                               GLbitfield mask,
                                                               GLenum filter);

// GL_EXT_multisampled_render_to_texture
typedef void(INTERNAL_GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(
    GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(GLenum target,
                                                                                GLenum attachment,
                                                                                GLenum textarget,
                                                                                GLuint texture,
                                                                                GLint level,
                                                                                GLsizei samples);

// GL_IMG_multisampled_render_to_texture
typedef void(INTERNAL_GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMGPROC)(
    GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMGPROC)(GLenum target,
                                                                                GLenum attachment,
                                                                                GLenum textarget,
                                                                                GLuint texture,
                                                                                GLint level,
                                                                                GLsizei samples);

// GL_NV_polygon_mode
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOLYGONMODENVPROC)(GLenum face, GLenum mode);

// GL_EXT_polygon_offset_clamp
typedef void(INTERNAL_GL_APIENTRY* PFNGLPOLYGONOFFSETCLAMPEXTPROC)(GLfloat factor,
                                                                   GLfloat units,
                                                                   GLfloat clamp);

// GL_EXT_shader_framebuffer_fetch_non_coherent
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERFETCHBARRIEREXTPROC)();

// GL_MESA_framebuffer_flip_y
typedef void(INTERNAL_GL_APIENTRY* PFNGLFRAMEBUFFERPARAMETERIMESAPROC)(GLenum, GLenum, GLint);

// 1.0
#define GL_ALPHA 0x1906
#define GL_ALWAYS 0x0207
#define GL_AND 0x1501
#define GL_AND_INVERTED 0x1504
#define GL_AND_REVERSE 0x1502
#define GL_BACK 0x0405
#define GL_BACK_LEFT 0x0402
#define GL_BACK_RIGHT 0x0403
#define GL_BLEND 0x0BE2
#define GL_BLEND_DST 0x0BE0
#define GL_BLEND_SRC 0x0BE1
#define GL_BLUE 0x1905
#define GL_BYTE 0x1400
#define GL_CCW 0x0901
#define GL_CLEAR 0x1500
#define GL_COLOR 0x1800
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_COPY 0x1503
#define GL_COPY_INVERTED 0x150C
#define GL_CULL_FACE 0x0B44
#define GL_CULL_FACE_MODE 0x0B45
#define GL_CW 0x0900
#define GL_DECR 0x1E03
#define GL_DEPTH 0x1801
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_FUNC 0x0B74
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_TEST 0x0B71
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DITHER 0x0BD0
#define GL_DONT_CARE 0x1100
#define GL_DOUBLE 0x140A
#define GL_DOUBLEBUFFER 0x0C32
#define GL_DRAW_BUFFER 0x0C01
#define GL_DST_ALPHA 0x0304
#define GL_DST_COLOR 0x0306
#define GL_EQUAL 0x0202
#define GL_EQUIV 0x1509
#define GL_EXTENSIONS 0x1F03
#define GL_FALSE 0
#define GL_FASTEST 0x1101
#define GL_FILL 0x1B02
#define GL_FLOAT 0x1406
#define GL_FRONT 0x0404
#define GL_FRONT_AND_BACK 0x0408
#define GL_FRONT_FACE 0x0B46
#define GL_FRONT_LEFT 0x0400
#define GL_FRONT_RIGHT 0x0401
#define GL_GEQUAL 0x0206
#define GL_GREATER 0x0204
#define GL_GREEN 0x1904
#define GL_INCR 0x1E02
#define GL_INT 0x1404
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_OPERATION 0x0502
#define GL_INVALID_VALUE 0x0501
#define GL_INVERT 0x150A
#define GL_KEEP 0x1E00
#define GL_LEFT 0x0406
#define GL_LEQUAL 0x0203
#define GL_LESS 0x0201
#define GL_LINE 0x1B01
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_SMOOTH 0x0B20
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_LINE_STRIP 0x0003
#define GL_LINE_WIDTH 0x0B21
#define GL_LINE_WIDTH_GRANULARITY 0x0B23
#define GL_LINE_WIDTH_RANGE 0x0B22
#define GL_LOGIC_OP_MODE 0x0BF0
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_NAND 0x150E
#define GL_NEAREST 0x2600
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_NEVER 0x0200
#define GL_NICEST 0x1102
#define GL_NONE 0
#define GL_NOOP 0x1505
#define GL_NOR 0x1508
#define GL_NOTEQUAL 0x0205
#define GL_NO_ERROR 0
#define GL_ONE 1
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_OR 0x1507
#define GL_OR_INVERTED 0x150D
#define GL_OR_REVERSE 0x150B
#define GL_OUT_OF_MEMORY 0x0505
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_PACK_LSB_FIRST 0x0D01
#define GL_PACK_ROW_LENGTH 0x0D02
#define GL_PACK_SKIP_PIXELS 0x0D04
#define GL_PACK_SKIP_ROWS 0x0D03
#define GL_PACK_SWAP_BYTES 0x0D00
#define GL_POINT 0x1B00
#define GL_POINTS 0x0000
#define GL_POINT_SIZE 0x0B11
#define GL_POINT_SIZE_GRANULARITY 0x0B13
#define GL_POINT_SIZE_RANGE 0x0B12
#define GL_POLYGON_MODE 0x0B40
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_POLYGON_OFFSET_LINE 0x2A02
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_SMOOTH 0x0B41
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#define GL_PROXY_TEXTURE_1D 0x8063
#define GL_PROXY_TEXTURE_2D 0x8064
#define GL_QUADS 0x0007
#define GL_R3_G3_B2 0x2A10
#define GL_READ_BUFFER 0x0C02
#define GL_RED 0x1903
#define GL_RENDERER 0x1F01
#define GL_REPEAT 0x2901
#define GL_REPLACE 0x1E01
#define GL_RGB 0x1907
#define GL_RGB10 0x8052
#define GL_RGB10_A2 0x8059
#define GL_RGB12 0x8053
#define GL_RGB16 0x8054
#define GL_RGB4 0x804F
#define GL_RGB5 0x8050
#define GL_RGB5_A1 0x8057
#define GL_RGB8 0x8051
#define GL_RGBA 0x1908
#define GL_RGBA12 0x805A
#define GL_RGBA16 0x805B
#define GL_RGBA2 0x8055
#define GL_RGBA4 0x8056
#define GL_RGBA8 0x8058
#define GL_RIGHT 0x0407
#define GL_SCISSOR_BOX 0x0C10
#define GL_SCISSOR_TEST 0x0C11
#define GL_SET 0x150F
#define GL_SHORT 0x1402
#define GL_SRC_ALPHA 0x0302
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_SRC_COLOR 0x0300
#define GL_STENCIL 0x1802
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_INDEX 0x1901
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_TEST 0x0B90
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_STEREO 0x0C33
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_TEXTURE 0x1702
#define GL_TEXTURE_1D 0x0DE0
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#define GL_TEXTURE_BINDING_1D 0x8068
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_TEXTURE_BLUE_SIZE 0x805E
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_TEXTURE_GREEN_SIZE 0x805D
#define GL_TEXTURE_HEIGHT 0x1001
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_RED_SIZE 0x805C
#define GL_TEXTURE_WIDTH 0x1000
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_FAN 0x0006
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRUE 1
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_UNPACK_LSB_FIRST 0x0CF1
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#define GL_UNPACK_SWAP_BYTES 0x0CF0
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_UNSIGNED_SHORT 0x1403
#define GL_VENDOR 0x1F00
#define GL_VERSION 0x1F02
#define GL_VIEWPORT 0x0BA2
#define GL_XOR 0x1506
#define GL_ZERO 0

// 1.2
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_PACK_IMAGE_HEIGHT 0x806C
#define GL_PACK_SKIP_IMAGES 0x806B
#define GL_PROXY_TEXTURE_3D 0x8070
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY 0x0B23
#define GL_SMOOTH_LINE_WIDTH_RANGE 0x0B22
#define GL_SMOOTH_POINT_SIZE_GRANULARITY 0x0B13
#define GL_SMOOTH_POINT_SIZE_RANGE 0x0B12
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_BINDING_3D 0x806A
#define GL_TEXTURE_DEPTH 0x8071
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#define GL_UNPACK_SKIP_IMAGES 0x806D
#define GL_UNSIGNED_BYTE_2_3_3_REV 0x8362
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#define GL_UNSIGNED_INT_10_10_10_2 0x8036
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364

// 1.2 Extensions
#define GL_ALL_COMPLETED_NV 0x84F2
#define GL_FENCE_STATUS_NV 0x84F3
#define GL_FENCE_CONDITION_NV 0x84F4

// 1.3
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_COMPRESSED_RGB 0x84ED
#define GL_COMPRESSED_RGBA 0x84EE
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#define GL_MULTISAMPLE 0x809D
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_PROXY_TEXTURE_CUBE_MAP 0x851B
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE 0x809F
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLE_COVERAGE 0x80A0
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1
#define GL_TEXTURE18 0x84D2
#define GL_TEXTURE19 0x84D3
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE20 0x84D4
#define GL_TEXTURE21 0x84D5
#define GL_TEXTURE22 0x84D6
#define GL_TEXTURE23 0x84D7
#define GL_TEXTURE24 0x84D8
#define GL_TEXTURE25 0x84D9
#define GL_TEXTURE26 0x84DA
#define GL_TEXTURE27 0x84DB
#define GL_TEXTURE28 0x84DC
#define GL_TEXTURE29 0x84DD
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE30 0x84DE
#define GL_TEXTURE31 0x84DF
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_COMPRESSED 0x86A1
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE 0x86A0
#define GL_TEXTURE_COMPRESSION_HINT 0x84EF
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519

// 1.5
#define GL_BLEND_COLOR 0x8005
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_EQUATION 0x8009
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_CONSTANT_ALPHA 0x8003
#define GL_CONSTANT_COLOR 0x8001
#define GL_DECR_WRAP 0x8508
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32 0x81A7
#define GL_FUNC_ADD 0x8006
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_FUNC_SUBTRACT 0x800A
#define GL_INCR_WRAP 0x8507
#define GL_MAX 0x8008
#define GL_MAX_TEXTURE_LOD_BIAS 0x84FD
#define GL_MIN 0x8007
#define GL_MIRRORED_REPEAT 0x8370
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_POINT_FADE_THRESHOLD_SIZE 0x8128
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_DEPTH_SIZE 0x884A
#define GL_TEXTURE_LOD_BIAS 0x8501

// 1.5
#define GL_ARRAY_BUFFER 0x8892
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_BUFFER_ACCESS 0x88BB
#define GL_BUFFER_MAPPED 0x88BC
#define GL_BUFFER_MAP_POINTER 0x88BD
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_CURRENT_QUERY 0x8865
#define GL_DYNAMIC_COPY 0x88EA
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_QUERY_COUNTER_BITS 0x8864
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_READ_ONLY 0x88B8
#define GL_READ_WRITE 0x88BA
#define GL_SAMPLES_PASSED 0x8914
#define GL_SRC1_ALPHA 0x8589
#define GL_STATIC_COPY 0x88E6
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STREAM_COPY 0x88E2
#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_WRITE_ONLY 0x88B9

// 2.0
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BOOL 0x8B56
#define GL_BOOL_VEC2 0x8B57
#define GL_BOOL_VEC3 0x8B58
#define GL_BOOL_VEC4 0x8B59
#define GL_COMPILE_STATUS 0x8B81
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_CURRENT_VERTEX_ATTRIB 0x8626
#define GL_DELETE_STATUS 0x8B80
#define GL_DRAW_BUFFER0 0x8825
#define GL_DRAW_BUFFER1 0x8826
#define GL_DRAW_BUFFER10 0x882F
#define GL_DRAW_BUFFER11 0x8830
#define GL_DRAW_BUFFER12 0x8831
#define GL_DRAW_BUFFER13 0x8832
#define GL_DRAW_BUFFER14 0x8833
#define GL_DRAW_BUFFER15 0x8834
#define GL_DRAW_BUFFER2 0x8827
#define GL_DRAW_BUFFER3 0x8828
#define GL_DRAW_BUFFER4 0x8829
#define GL_DRAW_BUFFER5 0x882A
#define GL_DRAW_BUFFER6 0x882B
#define GL_DRAW_BUFFER7 0x882C
#define GL_DRAW_BUFFER8 0x882D
#define GL_DRAW_BUFFER9 0x882E
#define GL_FLOAT_MAT2 0x8B5A
#define GL_FLOAT_MAT3 0x8B5B
#define GL_FLOAT_MAT4 0x8B5C
#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_INT_VEC2 0x8B53
#define GL_INT_VEC3 0x8B54
#define GL_INT_VEC4 0x8B55
#define GL_LINK_STATUS 0x8B82
#define GL_LOWER_LEFT 0x8CA1
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_DRAW_BUFFERS 0x8824
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_MAX_VARYING_FLOATS 0x8B4B
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS 0x8B4A
#define GL_POINT_SPRITE 0x8861
#define GL_POINT_SPRITE_COORD_ORIGIN 0x8CA0
#define GL_SAMPLER_1D 0x8B5D
#define GL_SAMPLER_1D_SHADOW 0x8B61
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_2D_SHADOW 0x8B62
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_CUBE 0x8B60
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#define GL_SHADER_TYPE 0x8B4F
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_STENCIL_BACK_FAIL 0x8801
#define GL_STENCIL_BACK_FUNC 0x8800
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#define GL_STENCIL_BACK_REF 0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK 0x8CA4
#define GL_STENCIL_BACK_WRITEMASK 0x8CA5
#define GL_UPPER_LEFT 0x8CA2
#define GL_VALIDATE_STATUS 0x8B83
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642
#define GL_VERTEX_SHADER 0x8B31

// 2.1
#define GL_COMPRESSED_SRGB 0x8C48
#define GL_COMPRESSED_SRGB_ALPHA 0x8C49
#define GL_FLOAT_MAT2x3 0x8B65
#define GL_FLOAT_MAT2x4 0x8B66
#define GL_FLOAT_MAT3x2 0x8B67
#define GL_FLOAT_MAT3x4 0x8B68
#define GL_FLOAT_MAT4x2 0x8B69
#define GL_FLOAT_MAT4x3 0x8B6A
#define GL_PIXEL_PACK_BUFFER 0x88EB
#define GL_PIXEL_PACK_BUFFER_BINDING 0x88ED
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#define GL_SRGB 0x8C40
#define GL_SRGB8 0x8C41
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_SRGB_ALPHA 0x8C42

// 3.0
#define GL_BGRA_INTEGER 0x8D9B
#define GL_BGR_INTEGER 0x8D9A
#define GL_BLUE_INTEGER 0x8D96
#define GL_BUFFER_ACCESS_FLAGS 0x911F
#define GL_BUFFER_MAP_LENGTH 0x9120
#define GL_BUFFER_MAP_OFFSET 0x9121
#define GL_CLAMP_READ_COLOR 0x891C
#define GL_CLIP_DISTANCE0 0x3000
#define GL_CLIP_DISTANCE1 0x3001
#define GL_CLIP_DISTANCE2 0x3002
#define GL_CLIP_DISTANCE3 0x3003
#define GL_CLIP_DISTANCE4 0x3004
#define GL_CLIP_DISTANCE5 0x3005
#define GL_CLIP_DISTANCE6 0x3006
#define GL_CLIP_DISTANCE7 0x3007
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT10 0x8CEA
#define GL_COLOR_ATTACHMENT11 0x8CEB
#define GL_COLOR_ATTACHMENT12 0x8CEC
#define GL_COLOR_ATTACHMENT13 0x8CED
#define GL_COLOR_ATTACHMENT14 0x8CEE
#define GL_COLOR_ATTACHMENT15 0x8CEF
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#define GL_COLOR_ATTACHMENT8 0x8CE8
#define GL_COLOR_ATTACHMENT9 0x8CE9
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_COMPRESSED_RED 0x8225
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#define GL_COMPRESSED_RG 0x8226
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define GL_CONTEXT_FLAGS 0x821E
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x00000001
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_DEPTH32F_STENCIL8 0x8CAD
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_DEPTH_STENCIL 0x84F9
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_FIXED_ONLY 0x891D
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#define GL_FRAMEBUFFER 0x8D40
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING 0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE 0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_DEFAULT 0x8218
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_GREEN_INTEGER 0x8D95
#define GL_HALF_FLOAT 0x140B
#define GL_INTERLEAVED_ATTRIBS 0x8C8C
#define GL_INT_SAMPLER_1D 0x8DC9
#define GL_INT_SAMPLER_1D_ARRAY 0x8DCE
#define GL_INT_SAMPLER_2D 0x8DCA
#define GL_INT_SAMPLER_2D_ARRAY 0x8DCF
#define GL_INT_SAMPLER_3D 0x8DCB
#define GL_INT_SAMPLER_CUBE 0x8DCC
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#define GL_MAJOR_VERSION 0x821B
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#define GL_MAX_CLIP_DISTANCES 0x0D32
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#define GL_MAX_PROGRAM_TEXEL_OFFSET 0x8905
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_MAX_SAMPLES 0x8D57
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS 0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS 0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS 0x8C80
#define GL_MAX_VARYING_COMPONENTS 0x8B4B
#define GL_MINOR_VERSION 0x821C
#define GL_MIN_PROGRAM_TEXEL_OFFSET 0x8904
#define GL_NUM_EXTENSIONS 0x821D
#define GL_PRIMITIVES_GENERATED 0x8C87
#define GL_PROXY_TEXTURE_1D_ARRAY 0x8C19
#define GL_PROXY_TEXTURE_2D_ARRAY 0x8C1B
#define GL_QUERY_BY_REGION_NO_WAIT 0x8E16
#define GL_QUERY_BY_REGION_WAIT 0x8E15
#define GL_QUERY_NO_WAIT 0x8E14
#define GL_QUERY_WAIT 0x8E13
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_R16 0x822A
#define GL_R16F 0x822D
#define GL_R16I 0x8233
#define GL_R16UI 0x8234
#define GL_R32F 0x822E
#define GL_R32I 0x8235
#define GL_R32UI 0x8236
#define GL_R8 0x8229
#define GL_R8I 0x8231
#define GL_R8UI 0x8232
#define GL_RASTERIZER_DISCARD 0x8C89
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_RED_INTEGER 0x8D94
#define GL_RENDERBUFFER 0x8D41
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RG 0x8227
#define GL_RG16 0x822C
#define GL_RG16F 0x822F
#define GL_RG16I 0x8239
#define GL_RG16UI 0x823A
#define GL_RG32F 0x8230
#define GL_RG32I 0x823B
#define GL_RG32UI 0x823C
#define GL_RG8 0x822B
#define GL_RG8I 0x8237
#define GL_RG8UI 0x8238
#define GL_RGB16F 0x881B
#define GL_RGB16I 0x8D89
#define GL_RGB16UI 0x8D77
#define GL_RGB32F 0x8815
#define GL_RGB32I 0x8D83
#define GL_RGB32UI 0x8D71
#define GL_RGB8I 0x8D8F
#define GL_RGB8UI 0x8D7D
#define GL_RGB9_E5 0x8C3D
#define GL_RGBA16F 0x881A
#define GL_RGBA16I 0x8D88
#define GL_RGBA16UI 0x8D76
#define GL_RGBA32F 0x8814
#define GL_RGBA32I 0x8D82
#define GL_RGBA32UI 0x8D70
#define GL_RGBA8I 0x8D8E
#define GL_RGBA8UI 0x8D7C
#define GL_RGBA_INTEGER 0x8D99
#define GL_RGB_INTEGER 0x8D98
#define GL_RG_INTEGER 0x8228
#define GL_SAMPLER_1D_ARRAY 0x8DC0
#define GL_SAMPLER_1D_ARRAY_SHADOW 0x8DC3
#define GL_SAMPLER_2D_ARRAY 0x8DC1
#define GL_SAMPLER_2D_ARRAY_SHADOW 0x8DC4
#define GL_SAMPLER_CUBE_SHADOW 0x8DC5
#define GL_SEPARATE_ATTRIBS 0x8C8D
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_STENCIL_INDEX1 0x8D46
#define GL_STENCIL_INDEX16 0x8D49
#define GL_STENCIL_INDEX4 0x8D47
#define GL_STENCIL_INDEX8 0x8D48
#define GL_TEXTURE_1D_ARRAY 0x8C18
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TEXTURE_ALPHA_TYPE 0x8C13
#define GL_TEXTURE_BINDING_1D_ARRAY 0x8C1C
#define GL_TEXTURE_BINDING_2D_ARRAY 0x8C1D
#define GL_TEXTURE_BLUE_TYPE 0x8C12
#define GL_TEXTURE_DEPTH_TYPE 0x8C16
#define GL_TEXTURE_GREEN_TYPE 0x8C11
#define GL_TEXTURE_RED_TYPE 0x8C10
#define GL_TEXTURE_SHARED_SIZE 0x8C3F
#define GL_TEXTURE_STENCIL_SIZE 0x88F1
#define GL_TRANSFORM_FEEDBACK_BUFFER 0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING 0x8C8F
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE 0x8C7F
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE 0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_START 0x8C84
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN 0x8C88
#define GL_TRANSFORM_FEEDBACK_VARYINGS 0x8C83
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH 0x8C76
#define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define GL_UNSIGNED_INT_SAMPLER_1D 0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY 0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_2D 0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY 0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_3D 0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE 0x8DD4
#define GL_UNSIGNED_INT_VEC2 0x8DC6
#define GL_UNSIGNED_INT_VEC3 0x8DC7
#define GL_UNSIGNED_INT_VEC4 0x8DC8
#define GL_UNSIGNED_NORMALIZED 0x8C17
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER 0x88FD

// 3.1
#define GL_ACTIVE_UNIFORM_BLOCKS 0x8A36
#define GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH 0x8A35
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_COPY_WRITE_BUFFER 0x8F37
#define GL_INT_SAMPLER_2D_RECT 0x8DCD
#define GL_INT_SAMPLER_BUFFER 0x8DD0
#define GL_INVALID_INDEX 0xFFFFFFFFu
#define GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS 0x8A33
#define GL_MAX_COMBINED_UNIFORM_BLOCKS 0x8A2E
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS 0x8A31
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS 0x8A2D
#define GL_MAX_RECTANGLE_TEXTURE_SIZE 0x84F8
#define GL_MAX_TEXTURE_BUFFER_SIZE 0x8C2B
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#define GL_MAX_UNIFORM_BUFFER_BINDINGS 0x8A2F
#define GL_MAX_VERTEX_UNIFORM_BLOCKS 0x8A2B
#define GL_PRIMITIVE_RESTART 0x8F9D
#define GL_PRIMITIVE_RESTART_INDEX 0x8F9E
#define GL_PROXY_TEXTURE_RECTANGLE 0x84F7
#define GL_R16_SNORM 0x8F98
#define GL_R8_SNORM 0x8F94
#define GL_RG16_SNORM 0x8F99
#define GL_RG8_SNORM 0x8F95
#define GL_RGB16_SNORM 0x8F9A
#define GL_RGB8_SNORM 0x8F96
#define GL_RGBA16_SNORM 0x8F9B
#define GL_RGBA8_SNORM 0x8F97
#define GL_SAMPLER_2D_RECT 0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW 0x8B64
#define GL_SAMPLER_BUFFER 0x8DC2
#define GL_SIGNED_NORMALIZED 0x8F9C
#define GL_TEXTURE_BINDING_BUFFER 0x8C2C
#define GL_TEXTURE_BINDING_RECTANGLE 0x84F6
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING 0x8C2D
#define GL_TEXTURE_RECTANGLE 0x84F5
#define GL_UNIFORM_ARRAY_STRIDE 0x8A3C
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS 0x8A42
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES 0x8A43
#define GL_UNIFORM_BLOCK_BINDING 0x8A3F
#define GL_UNIFORM_BLOCK_DATA_SIZE 0x8A40
#define GL_UNIFORM_BLOCK_INDEX 0x8A3A
#define GL_UNIFORM_BLOCK_NAME_LENGTH 0x8A41
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER 0x8A46
#define GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER 0x8A44
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_UNIFORM_BUFFER_BINDING 0x8A28
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x8A34
#define GL_UNIFORM_BUFFER_SIZE 0x8A2A
#define GL_UNIFORM_BUFFER_START 0x8A29
#define GL_UNIFORM_IS_ROW_MAJOR 0x8A3E
#define GL_UNIFORM_MATRIX_STRIDE 0x8A3D
#define GL_UNIFORM_NAME_LENGTH 0x8A39
#define GL_UNIFORM_OFFSET 0x8A3B
#define GL_UNIFORM_SIZE 0x8A38
#define GL_UNIFORM_TYPE 0x8A37
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT 0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8

// 3.2
#define GL_ALREADY_SIGNALED 0x911A
#define GL_CONDITION_SATISFIED 0x911C
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#define GL_CONTEXT_PROFILE_MASK 0x9126
#define GL_DEPTH_CLAMP 0x864F
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED 0x8DA7
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#define GL_GEOMETRY_INPUT_TYPE 0x8917
#define GL_GEOMETRY_OUTPUT_TYPE 0x8918
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_GEOMETRY_VERTICES_OUT 0x8916
#define GL_INT_SAMPLER_2D_MULTISAMPLE 0x9109
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#define GL_LAST_VERTEX_CONVENTION 0x8E4E
#define GL_LINES_ADJACENCY 0x000A
#define GL_LINE_STRIP_ADJACENCY 0x000B
#define GL_MAX_COLOR_TEXTURE_SAMPLES 0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES 0x910F
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS 0x9125
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES 0x8DE0
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS 0x8DE1
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS 0x8DDF
#define GL_MAX_INTEGER_SAMPLES 0x9110
#define GL_MAX_SAMPLE_MASK_WORDS 0x8E59
#define GL_MAX_SERVER_WAIT_TIMEOUT 0x9111
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS 0x9122
#define GL_OBJECT_TYPE 0x9112
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_PROVOKING_VERTEX 0x8E4F
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE 0x9101
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9103
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION 0x8E4C
#define GL_SAMPLER_2D_MULTISAMPLE 0x9108
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910B
#define GL_SAMPLE_MASK 0x8E51
#define GL_SAMPLE_MASK_VALUE 0x8E52
#define GL_SAMPLE_POSITION 0x8E50
#define GL_SIGNALED 0x9119
#define GL_SYNC_CONDITION 0x9113
#define GL_SYNC_FENCE 0x9116
#define GL_SYNC_FLAGS 0x9115
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_SYNC_STATUS 0x9114
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS 0x9107
#define GL_TEXTURE_SAMPLES 0x9106
#define GL_TIMEOUT_EXPIRED 0x911B
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFFull
#define GL_TRIANGLES_ADJACENCY 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY 0x000D
#define GL_UNSIGNALED 0x9118
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#define GL_WAIT_FAILED 0x911D

// 3.3
#define GL_ANY_SAMPLES_PASSED 0x8C2F
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS 0x88FC
#define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#define GL_RGB10_A2UI 0x906F
#define GL_SAMPLER_BINDING 0x8919
#define GL_SRC1_COLOR 0x88F9
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#define GL_TIMESTAMP 0x8E28
#define GL_TIME_ELAPSED 0x88BF
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR 0x88FE

// 4.0
#define GL_ACTIVE_SUBROUTINES 0x8DE5
#define GL_ACTIVE_SUBROUTINE_MAX_LENGTH 0x8E48
#define GL_ACTIVE_SUBROUTINE_UNIFORMS 0x8DE6
#define GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS 0x8E47
#define GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH 0x8E49
#define GL_COMPATIBLE_SUBROUTINES 0x8E4B
#define GL_DOUBLE_MAT2 0x8F46
#define GL_DOUBLE_MAT2x3 0x8F49
#define GL_DOUBLE_MAT2x4 0x8F4A
#define GL_DOUBLE_MAT3 0x8F47
#define GL_DOUBLE_MAT3x2 0x8F4B
#define GL_DOUBLE_MAT3x4 0x8F4C
#define GL_DOUBLE_MAT4 0x8F48
#define GL_DOUBLE_MAT4x2 0x8F4D
#define GL_DOUBLE_MAT4x3 0x8F4E
#define GL_DOUBLE_VEC2 0x8FFC
#define GL_DOUBLE_VEC3 0x8FFD
#define GL_DOUBLE_VEC4 0x8FFE
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#define GL_DRAW_INDIRECT_BUFFER_BINDING 0x8F43
#define GL_FRACTIONAL_EVEN 0x8E7C
#define GL_FRACTIONAL_ODD 0x8E7B
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS 0x8E5D
#define GL_GEOMETRY_SHADER_INVOCATIONS 0x887F
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY 0x900E
#define GL_ISOLINES 0x8E7A
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E1F
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET 0x8E5C
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS 0x8E5A
#define GL_MAX_PATCH_VERTICES 0x8E7D
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET 0x8E5F
#define GL_MAX_SUBROUTINES 0x8DE7
#define GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS 0x8DE8
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS 0x886C
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS 0x8E83
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS 0x8E81
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS 0x8E85
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS 0x8E89
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E7F
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS 0x886D
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS 0x8E86
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS 0x8E82
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS 0x8E8A
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E80
#define GL_MAX_TESS_GEN_LEVEL 0x8E7E
#define GL_MAX_TESS_PATCH_COMPONENTS 0x8E84
#define GL_MAX_TRANSFORM_FEEDBACK_BUFFERS 0x8E70
#define GL_MAX_VERTEX_STREAMS 0x8E71
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET 0x8E5B
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET 0x8E5E
#define GL_MIN_SAMPLE_SHADING_VALUE 0x8C37
#define GL_NUM_COMPATIBLE_SUBROUTINES 0x8E4A
#define GL_PATCHES 0x000E
#define GL_PATCH_DEFAULT_INNER_LEVEL 0x8E73
#define GL_PATCH_DEFAULT_OUTER_LEVEL 0x8E74
#define GL_PATCH_VERTICES 0x8E72
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY 0x900B
#define GL_SAMPLER_CUBE_MAP_ARRAY 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW 0x900D
#define GL_SAMPLE_SHADING 0x8C36
#define GL_TESS_CONTROL_OUTPUT_VERTICES 0x8E75
#define GL_TESS_CONTROL_SHADER 0x8E88
#define GL_TESS_EVALUATION_SHADER 0x8E87
#define GL_TESS_GEN_MODE 0x8E76
#define GL_TESS_GEN_POINT_MODE 0x8E79
#define GL_TESS_GEN_SPACING 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER 0x8E78
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY 0x900A
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#define GL_TRANSFORM_FEEDBACK 0x8E22
#define GL_TRANSFORM_FEEDBACK_BINDING 0x8E25
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE 0x8E24
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED 0x8E23
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER 0x84F0
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER 0x84F1
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY 0x900F

// 4.1
#define GL_ACTIVE_PROGRAM 0x8259
#define GL_ALL_SHADER_BITS 0xFFFFFFFF
#define GL_FIXED 0x140C
#define GL_FRAGMENT_SHADER_BIT 0x00000002
#define GL_GEOMETRY_SHADER_BIT 0x00000004
#define GL_HIGH_FLOAT 0x8DF2
#define GL_HIGH_INT 0x8DF5
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#define GL_IMPLEMENTATION_COLOR_READ_TYPE 0x8B9A
#define GL_LAYER_PROVOKING_VERTEX 0x825E
#define GL_LOW_FLOAT 0x8DF0
#define GL_LOW_INT 0x8DF3
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#define GL_MAX_VARYING_VECTORS 0x8DFC
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_MAX_VIEWPORTS 0x825B
#define GL_MEDIUM_FLOAT 0x8DF1
#define GL_MEDIUM_INT 0x8DF4
#define GL_NUM_PROGRAM_BINARY_FORMATS 0x87FE
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define GL_PROGRAM_BINARY_FORMATS 0x87FF
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 0x8257
#define GL_PROGRAM_PIPELINE_BINDING 0x825A
#define GL_PROGRAM_SEPARABLE 0x8258
#define GL_RGB565 0x8D62
#define GL_SHADER_BINARY_FORMATS 0x8DF8
#define GL_SHADER_COMPILER 0x8DFA
#define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#define GL_UNDEFINED_VERTEX 0x8260
#define GL_VERTEX_SHADER_BIT 0x00000001
#define GL_VIEWPORT_BOUNDS_RANGE 0x825D
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX 0x825F
#define GL_VIEWPORT_SUBPIXEL_BITS 0x825C

// 4.2
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS 0x92D9
#define GL_ALL_BARRIER_BITS 0xFFFFFFFF
#define GL_ATOMIC_COUNTER_BARRIER_BIT 0x00001000
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS 0x92C5
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES 0x92C6
#define GL_ATOMIC_COUNTER_BUFFER_BINDING 0x92C1
#define GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE 0x92C4
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_FRAGMENT_SHADER 0x92CB
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_GEOMETRY_SHADER 0x92CA
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_CONTROL_SHADER 0x92C8
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_EVALUATION_SHADER 0x92C9
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_VERTEX_SHADER 0x92C7
#define GL_ATOMIC_COUNTER_BUFFER_SIZE 0x92C3
#define GL_ATOMIC_COUNTER_BUFFER_START 0x92C2
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#define GL_COMMAND_BARRIER_BIT 0x00000040
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define GL_COPY_READ_BUFFER_BINDING 0x8F36
#define GL_COPY_WRITE_BUFFER_BINDING 0x8F37
#define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#define GL_IMAGE_1D 0x904C
#define GL_IMAGE_1D_ARRAY 0x9052
#define GL_IMAGE_2D 0x904D
#define GL_IMAGE_2D_ARRAY 0x9053
#define GL_IMAGE_2D_MULTISAMPLE 0x9055
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY 0x9056
#define GL_IMAGE_2D_RECT 0x904F
#define GL_IMAGE_3D 0x904E
#define GL_IMAGE_BINDING_ACCESS 0x8F3E
#define GL_IMAGE_BINDING_FORMAT 0x906E
#define GL_IMAGE_BINDING_LAYER 0x8F3D
#define GL_IMAGE_BINDING_LAYERED 0x8F3C
#define GL_IMAGE_BINDING_LEVEL 0x8F3B
#define GL_IMAGE_BINDING_NAME 0x8F3A
#define GL_IMAGE_BUFFER 0x9051
#define GL_IMAGE_CUBE 0x9050
#define GL_IMAGE_CUBE_MAP_ARRAY 0x9054
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS 0x90C9
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE 0x90C8
#define GL_IMAGE_FORMAT_COMPATIBILITY_TYPE 0x90C7
#define GL_INT_IMAGE_1D 0x9057
#define GL_INT_IMAGE_1D_ARRAY 0x905D
#define GL_INT_IMAGE_2D 0x9058
#define GL_INT_IMAGE_2D_ARRAY 0x905E
#define GL_INT_IMAGE_2D_MULTISAMPLE 0x9060
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x9061
#define GL_INT_IMAGE_2D_RECT 0x905A
#define GL_INT_IMAGE_3D 0x9059
#define GL_INT_IMAGE_BUFFER 0x905C
#define GL_INT_IMAGE_CUBE 0x905B
#define GL_INT_IMAGE_CUBE_MAP_ARRAY 0x905F
#define GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS 0x92DC
#define GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE 0x92D8
#define GL_MAX_COMBINED_ATOMIC_COUNTERS 0x92D7
#define GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS 0x92D1
#define GL_MAX_COMBINED_IMAGE_UNIFORMS 0x90CF
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS 0x8F39
#define GL_MAX_FRAGMENT_ATOMIC_COUNTERS 0x92D6
#define GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS 0x92D0
#define GL_MAX_FRAGMENT_IMAGE_UNIFORMS 0x90CE
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS 0x92D5
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS 0x92CF
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS 0x90CD
#define GL_MAX_IMAGE_SAMPLES 0x906D
#define GL_MAX_IMAGE_UNITS 0x8F38
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS 0x92D3
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS 0x92CD
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS 0x90CB
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS 0x92D4
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS 0x92CE
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS 0x90CC
#define GL_MAX_VERTEX_ATOMIC_COUNTERS 0x92D2
#define GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS 0x92CC
#define GL_MAX_VERTEX_IMAGE_UNIFORMS 0x90CA
#define GL_MIN_MAP_BUFFER_ALIGNMENT 0x90BC
#define GL_NUM_SAMPLE_COUNTS 0x9380
#define GL_PACK_COMPRESSED_BLOCK_DEPTH 0x912D
#define GL_PACK_COMPRESSED_BLOCK_HEIGHT 0x912C
#define GL_PACK_COMPRESSED_BLOCK_SIZE 0x912E
#define GL_PACK_COMPRESSED_BLOCK_WIDTH 0x912B
#define GL_PIXEL_BUFFER_BARRIER_BIT 0x00000080
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#define GL_TEXTURE_IMMUTABLE_FORMAT 0x912F
#define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#define GL_TRANSFORM_FEEDBACK_ACTIVE 0x8E24
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_TRANSFORM_FEEDBACK_PAUSED 0x8E23
#define GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX 0x92DA
#define GL_UNIFORM_BARRIER_BIT 0x00000004
#define GL_UNPACK_COMPRESSED_BLOCK_DEPTH 0x9129
#define GL_UNPACK_COMPRESSED_BLOCK_HEIGHT 0x9128
#define GL_UNPACK_COMPRESSED_BLOCK_SIZE 0x912A
#define GL_UNPACK_COMPRESSED_BLOCK_WIDTH 0x9127
#define GL_UNSIGNED_INT_ATOMIC_COUNTER 0x92DB
#define GL_UNSIGNED_INT_IMAGE_1D 0x9062
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY 0x9068
#define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY 0x9069
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE 0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x906C
#define GL_UNSIGNED_INT_IMAGE_2D_RECT 0x9065
#define GL_UNSIGNED_INT_IMAGE_3D 0x9064
#define GL_UNSIGNED_INT_IMAGE_BUFFER 0x9067
#define GL_UNSIGNED_INT_IMAGE_CUBE 0x9066
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001

// 4.3
#define GL_ACTIVE_RESOURCES 0x92F5
#define GL_ACTIVE_VARIABLES 0x9305
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE 0x8D6A
#define GL_ARRAY_SIZE 0x92FB
#define GL_ARRAY_STRIDE 0x92FE
#define GL_ATOMIC_COUNTER_BUFFER_INDEX 0x9301
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER 0x90ED
#define GL_AUTO_GENERATE_MIPMAP 0x8295
#define GL_BLOCK_INDEX 0x92FD
#define GL_BUFFER 0x82E0
#define GL_BUFFER_BINDING 0x9302
#define GL_BUFFER_DATA_SIZE 0x9303
#define GL_BUFFER_VARIABLE 0x92E5
#define GL_CAVEAT_SUPPORT 0x82B8
#define GL_CLEAR_BUFFER 0x82B4
#define GL_COLOR_COMPONENTS 0x8283
#define GL_COLOR_ENCODING 0x8296
#define GL_COLOR_RENDERABLE 0x8286
#define GL_COMPRESSED_R11_EAC 0x9270
#define GL_COMPRESSED_RG11_EAC 0x9272
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define GL_COMPUTE_SHADER 0x91B9
#define GL_COMPUTE_SHADER_BIT 0x00000020
#define GL_COMPUTE_SUBROUTINE 0x92ED
#define GL_COMPUTE_SUBROUTINE_UNIFORM 0x92F3
#define GL_COMPUTE_TEXTURE 0x82A0
#define GL_COMPUTE_WORK_GROUP_SIZE 0x8267
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#define GL_DEBUG_CALLBACK_FUNCTION 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM 0x8245
#define GL_DEBUG_GROUP_STACK_DEPTH 0x826D
#define GL_DEBUG_LOGGED_MESSAGES 0x9145
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_POP_GROUP 0x826A
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEPTH_COMPONENTS 0x8284
#define GL_DEPTH_RENDERABLE 0x8287
#define GL_DEPTH_STENCIL_TEXTURE_MODE 0x90EA
#define GL_DISPATCH_INDIRECT_BUFFER 0x90EE
#define GL_DISPATCH_INDIRECT_BUFFER_BINDING 0x90EF
#define GL_FILTER 0x829A
#define GL_FRAGMENT_SUBROUTINE 0x92EC
#define GL_FRAGMENT_SUBROUTINE_UNIFORM 0x92F2
#define GL_FRAGMENT_TEXTURE 0x829F
#define GL_FRAMEBUFFER_BLEND 0x828B
#define GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS 0x9314
#define GL_FRAMEBUFFER_DEFAULT_HEIGHT 0x9311
#define GL_FRAMEBUFFER_DEFAULT_LAYERS 0x9312
#define GL_FRAMEBUFFER_DEFAULT_SAMPLES 0x9313
#define GL_FRAMEBUFFER_DEFAULT_WIDTH 0x9310
#define GL_FRAMEBUFFER_RENDERABLE 0x8289
#define GL_FRAMEBUFFER_RENDERABLE_LAYERED 0x828A
#define GL_FULL_SUPPORT 0x82B7
#define GL_GEOMETRY_SUBROUTINE 0x92EB
#define GL_GEOMETRY_SUBROUTINE_UNIFORM 0x92F1
#define GL_GEOMETRY_TEXTURE 0x829E
#define GL_GET_TEXTURE_IMAGE_FORMAT 0x8291
#define GL_GET_TEXTURE_IMAGE_TYPE 0x8292
#define GL_IMAGE_CLASS_10_10_10_2 0x82C3
#define GL_IMAGE_CLASS_11_11_10 0x82C2
#define GL_IMAGE_CLASS_1_X_16 0x82BE
#define GL_IMAGE_CLASS_1_X_32 0x82BB
#define GL_IMAGE_CLASS_1_X_8 0x82C1
#define GL_IMAGE_CLASS_2_X_16 0x82BD
#define GL_IMAGE_CLASS_2_X_32 0x82BA
#define GL_IMAGE_CLASS_2_X_8 0x82C0
#define GL_IMAGE_CLASS_4_X_16 0x82BC
#define GL_IMAGE_CLASS_4_X_32 0x82B9
#define GL_IMAGE_CLASS_4_X_8 0x82BF
#define GL_IMAGE_COMPATIBILITY_CLASS 0x82A8
#define GL_IMAGE_PIXEL_FORMAT 0x82A9
#define GL_IMAGE_PIXEL_TYPE 0x82AA
#define GL_IMAGE_TEXEL_SIZE 0x82A7
#define GL_INTERNALFORMAT_ALPHA_SIZE 0x8274
#define GL_INTERNALFORMAT_ALPHA_TYPE 0x827B
#define GL_INTERNALFORMAT_BLUE_SIZE 0x8273
#define GL_INTERNALFORMAT_BLUE_TYPE 0x827A
#define GL_INTERNALFORMAT_DEPTH_SIZE 0x8275
#define GL_INTERNALFORMAT_DEPTH_TYPE 0x827C
#define GL_INTERNALFORMAT_GREEN_SIZE 0x8272
#define GL_INTERNALFORMAT_GREEN_TYPE 0x8279
#define GL_INTERNALFORMAT_PREFERRED 0x8270
#define GL_INTERNALFORMAT_RED_SIZE 0x8271
#define GL_INTERNALFORMAT_RED_TYPE 0x8278
#define GL_INTERNALFORMAT_SHARED_SIZE 0x8277
#define GL_INTERNALFORMAT_STENCIL_SIZE 0x8276
#define GL_INTERNALFORMAT_STENCIL_TYPE 0x827D
#define GL_INTERNALFORMAT_SUPPORTED 0x826F
#define GL_IS_PER_PATCH 0x92E7
#define GL_IS_ROW_MAJOR 0x9300
#define GL_LOCATION 0x930E
#define GL_LOCATION_INDEX 0x930F
#define GL_MANUAL_GENERATE_MIPMAP 0x8294
#define GL_MATRIX_STRIDE 0x92FF
#define GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS 0x8266
#define GL_MAX_COMBINED_DIMENSIONS 0x8282
#define GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES 0x8F39
#define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS 0x90DC
#define GL_MAX_COMPUTE_ATOMIC_COUNTERS 0x8265
#define GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS 0x8264
#define GL_MAX_COMPUTE_IMAGE_UNIFORMS 0x91BD
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS 0x90DB
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE 0x8262
#define GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS 0x91BC
#define GL_MAX_COMPUTE_UNIFORM_BLOCKS 0x91BB
#define GL_MAX_COMPUTE_UNIFORM_COMPONENTS 0x8263
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT 0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE 0x91BF
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH 0x826C
#define GL_MAX_DEBUG_LOGGED_MESSAGES 0x9144
#define GL_MAX_DEBUG_MESSAGE_LENGTH 0x9143
#define GL_MAX_DEPTH 0x8280
#define GL_MAX_ELEMENT_INDEX 0x8D6B
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 0x90DA
#define GL_MAX_FRAMEBUFFER_HEIGHT 0x9316
#define GL_MAX_FRAMEBUFFER_LAYERS 0x9317
#define GL_MAX_FRAMEBUFFER_SAMPLES 0x9318
#define GL_MAX_FRAMEBUFFER_WIDTH 0x9315
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS 0x90D7
#define GL_MAX_HEIGHT 0x827F
#define GL_MAX_LABEL_LENGTH 0x82E8
#define GL_MAX_LAYERS 0x8281
#define GL_MAX_NAME_LENGTH 0x92F6
#define GL_MAX_NUM_ACTIVE_VARIABLES 0x92F7
#define GL_MAX_NUM_COMPATIBLE_SUBROUTINES 0x92F8
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x90DD
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS 0x90D9
#define GL_MAX_UNIFORM_LOCATIONS 0x826E
#define GL_MAX_VERTEX_ATTRIB_BINDINGS 0x82DA
#define GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET 0x82D9
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 0x90D6
#define GL_MAX_WIDTH 0x827E
#define GL_MIPMAP 0x8293
#define GL_NAME_LENGTH 0x92F9
#define GL_NUM_ACTIVE_VARIABLES 0x9304
#define GL_NUM_SHADING_LANGUAGE_VERSIONS 0x82E9
#define GL_OFFSET 0x92FC
#define GL_PRIMITIVE_RESTART_FIXED_INDEX 0x8D69
#define GL_PROGRAM 0x82E2
#define GL_PROGRAM_INPUT 0x92E3
#define GL_PROGRAM_OUTPUT 0x92E4
#define GL_PROGRAM_PIPELINE 0x82E4
#define GL_QUERY 0x82E3
#define GL_READ_PIXELS 0x828C
#define GL_READ_PIXELS_FORMAT 0x828D
#define GL_READ_PIXELS_TYPE 0x828E
#define GL_REFERENCED_BY_COMPUTE_SHADER 0x930B
#define GL_REFERENCED_BY_FRAGMENT_SHADER 0x930A
#define GL_REFERENCED_BY_GEOMETRY_SHADER 0x9309
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER 0x9308
#define GL_REFERENCED_BY_VERTEX_SHADER 0x9306
#define GL_SAMPLER 0x82E6
#define GL_SHADER 0x82E1
#define GL_SHADER_IMAGE_ATOMIC 0x82A6
#define GL_SHADER_IMAGE_LOAD 0x82A4
#define GL_SHADER_IMAGE_STORE 0x82A5
#define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#define GL_SHADER_STORAGE_BLOCK 0x92E6
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#define GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT 0x90DF
#define GL_SHADER_STORAGE_BUFFER_SIZE 0x90D5
#define GL_SHADER_STORAGE_BUFFER_START 0x90D4
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST 0x82AC
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE 0x82AE
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST 0x82AD
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE 0x82AF
#define GL_SRGB_READ 0x8297
#define GL_SRGB_WRITE 0x8298
#define GL_STENCIL_COMPONENTS 0x8285
#define GL_STENCIL_RENDERABLE 0x8288
#define GL_TESS_CONTROL_SUBROUTINE 0x92E9
#define GL_TESS_CONTROL_SUBROUTINE_UNIFORM 0x92EF
#define GL_TESS_CONTROL_TEXTURE 0x829C
#define GL_TESS_EVALUATION_SUBROUTINE 0x92EA
#define GL_TESS_EVALUATION_SUBROUTINE_UNIFORM 0x92F0
#define GL_TESS_EVALUATION_TEXTURE 0x829D
#define GL_TEXTURE_BUFFER_OFFSET 0x919D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT 0x919F
#define GL_TEXTURE_BUFFER_SIZE 0x919E
#define GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT 0x82B2
#define GL_TEXTURE_COMPRESSED_BLOCK_SIZE 0x82B3
#define GL_TEXTURE_COMPRESSED_BLOCK_WIDTH 0x82B1
#define GL_TEXTURE_GATHER 0x82A2
#define GL_TEXTURE_GATHER_SHADOW 0x82A3
#define GL_TEXTURE_IMAGE_FORMAT 0x828F
#define GL_TEXTURE_IMAGE_TYPE 0x8290
#define GL_TEXTURE_IMMUTABLE_LEVELS 0x82DF
#define GL_TEXTURE_SHADOW 0x82A1
#define GL_TEXTURE_VIEW 0x82B5
#define GL_TEXTURE_VIEW_MIN_LAYER 0x82DD
#define GL_TEXTURE_VIEW_MIN_LEVEL 0x82DB
#define GL_TEXTURE_VIEW_NUM_LAYERS 0x82DE
#define GL_TEXTURE_VIEW_NUM_LEVELS 0x82DC
#define GL_TOP_LEVEL_ARRAY_SIZE 0x930C
#define GL_TOP_LEVEL_ARRAY_STRIDE 0x930D
#define GL_TRANSFORM_FEEDBACK_VARYING 0x92F4
#define GL_TYPE 0x92FA
#define GL_UNIFORM 0x92E1
#define GL_UNIFORM_BLOCK 0x92E2
#define GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER 0x90EC
#define GL_VERTEX_ATTRIB_ARRAY_LONG 0x874E
#define GL_VERTEX_ATTRIB_BINDING 0x82D4
#define GL_VERTEX_ATTRIB_RELATIVE_OFFSET 0x82D5
#define GL_VERTEX_BINDING_BUFFER 0x8F4F
#define GL_VERTEX_BINDING_DIVISOR 0x82D6
#define GL_VERTEX_BINDING_OFFSET 0x82D7
#define GL_VERTEX_BINDING_STRIDE 0x82D8
#define GL_VERTEX_SUBROUTINE 0x92E8
#define GL_VERTEX_SUBROUTINE_UNIFORM 0x92EE
#define GL_VERTEX_TEXTURE 0x829B
#define GL_VIEW_CLASS_128_BITS 0x82C4
#define GL_VIEW_CLASS_16_BITS 0x82CA
#define GL_VIEW_CLASS_24_BITS 0x82C9
#define GL_VIEW_CLASS_32_BITS 0x82C8
#define GL_VIEW_CLASS_48_BITS 0x82C7
#define GL_VIEW_CLASS_64_BITS 0x82C6
#define GL_VIEW_CLASS_8_BITS 0x82CB
#define GL_VIEW_CLASS_96_BITS 0x82C5
#define GL_VIEW_CLASS_BPTC_FLOAT 0x82D3
#define GL_VIEW_CLASS_BPTC_UNORM 0x82D2
#define GL_VIEW_CLASS_RGTC1_RED 0x82D0
#define GL_VIEW_CLASS_RGTC2_RG 0x82D1
#define GL_VIEW_CLASS_S3TC_DXT1_RGB 0x82CC
#define GL_VIEW_CLASS_S3TC_DXT1_RGBA 0x82CD
#define GL_VIEW_CLASS_S3TC_DXT3_RGBA 0x82CE
#define GL_VIEW_CLASS_S3TC_DXT5_RGBA 0x82CF
#define GL_VIEW_COMPATIBILITY_CLASS 0x82B6

// 4.4
#define GL_BUFFER_IMMUTABLE_STORAGE 0x821F
#define GL_BUFFER_STORAGE_FLAGS 0x8220
#define GL_CLEAR_TEXTURE 0x9365
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#define GL_CLIENT_STORAGE_BIT 0x0200
#define GL_DYNAMIC_STORAGE_BIT 0x0100
#define GL_LOCATION_COMPONENT 0x934A
#define GL_MAP_COHERENT_BIT 0x0080
#define GL_MAP_PERSISTENT_BIT 0x0040
#define GL_MAX_VERTEX_ATTRIB_STRIDE 0x82E5
#define GL_MIRROR_CLAMP_TO_EDGE 0x8743
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED 0x8221
#define GL_QUERY_BUFFER 0x9192
#define GL_QUERY_BUFFER_BARRIER_BIT 0x00008000
#define GL_QUERY_BUFFER_BINDING 0x9193
#define GL_QUERY_RESULT_NO_WAIT 0x9194
#define GL_TEXTURE_BUFFER_BINDING 0x8C2A
#define GL_TRANSFORM_FEEDBACK_BUFFER_INDEX 0x934B
#define GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE 0x934C

// 4.5
#define GL_CLIP_DEPTH_MODE 0x935D
#define GL_CLIP_ORIGIN 0x935C
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT 0x00000004
#define GL_CONTEXT_LOST 0x0507
#define GL_CONTEXT_RELEASE_BEHAVIOR 0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH 0x82FC
#define GL_GUILTY_CONTEXT_RESET 0x8253
#define GL_INNOCENT_CONTEXT_RESET 0x8254
#define GL_LOSE_CONTEXT_ON_RESET 0x8252
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES 0x82FA
#define GL_MAX_CULL_DISTANCES 0x82F9
#define GL_NEGATIVE_ONE_TO_ONE 0x935E
#define GL_NO_RESET_NOTIFICATION 0x8261
#define GL_QUERY_BY_REGION_NO_WAIT_INVERTED 0x8E1A
#define GL_QUERY_BY_REGION_WAIT_INVERTED 0x8E19
#define GL_QUERY_NO_WAIT_INVERTED 0x8E18
#define GL_QUERY_TARGET 0x82EA
#define GL_QUERY_WAIT_INVERTED 0x8E17
#define GL_RESET_NOTIFICATION_STRATEGY 0x8256
#define GL_TEXTURE_TARGET 0x1006
#define GL_UNKNOWN_CONTEXT_RESET 0x8255
#define GL_ZERO_TO_ONE 0x935F
#define GL_COMPLETION_STATUS 0x91B1

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

}  // namespace gl
