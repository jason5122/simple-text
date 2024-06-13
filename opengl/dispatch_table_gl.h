#pragma once

#include "opengl/functionsgl_typedefs.h"
#include "util/non_copyable.h"
#include <string>

namespace opengl {

class DispatchTableGL : util::NonCopyable {
public:
    // 1.0
    PFNGLCLEARPROC clear = nullptr;
    PFNGLCLEARCOLORPROC clearColor = nullptr;
    PFNGLDEPTHMASKPROC depthMask = nullptr;
    PFNGLENABLEPROC enable = nullptr;
    PFNGLGETERRORPROC getError = nullptr;
    PFNGLVIEWPORTPROC viewport = nullptr;
    PFNGLPIXELSTOREIPROC pixelStorei = nullptr;
    PFNGLTEXIMAGE2DPROC texImage2D = nullptr;
    PFNGLTEXPARAMETERIPROC texParameteri = nullptr;

    // 1.1
    PFNGLGENTEXTURESPROC genTextures = nullptr;
    PFNGLDELETETEXTURESPROC deleteTextures = nullptr;
    PFNGLBINDTEXTUREPROC bindTexture = nullptr;
    PFNGLTEXSUBIMAGE2DPROC texSubImage2D = nullptr;

    // 1.3
    PFNGLACTIVETEXTUREPROC activeTexture = nullptr;

    // 1.4
    PFNGLBLENDFUNCSEPARATEPROC blendFuncSeparate = nullptr;

    // 1.5
    PFNGLGENBUFFERSPROC genBuffers = nullptr;
    PFNGLBINDBUFFERPROC bindBuffer = nullptr;
    PFNGLBUFFERDATAPROC bufferData = nullptr;
    PFNGLBUFFERSUBDATAPROC bufferSubData = nullptr;
    PFNGLDELETEBUFFERSPROC deleteBuffers = nullptr;

    // 2.0
    PFNGLATTACHSHADERPROC attachShader = nullptr;
    PFNGLCOMPILESHADERPROC compileShader = nullptr;
    PFNGLCREATEPROGRAMPROC createProgram = nullptr;
    PFNGLCREATESHADERPROC createShader = nullptr;
    PFNGLDELETEPROGRAMPROC deleteProgram = nullptr;
    PFNGLDELETESHADERPROC deleteShader = nullptr;
    PFNGLLINKPROGRAMPROC linkProgram = nullptr;
    PFNGLSHADERSOURCEPROC shaderSource = nullptr;
    PFNGLGETSHADERINFOLOGPROC getShaderInfoLog = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog = nullptr;
    PFNGLGETSHADERIVPROC getShaderiv = nullptr;
    PFNGLGETPROGRAMIVPROC getProgramiv = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer = nullptr;
    PFNGLUSEPROGRAMPROC useProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC getUniformLocation = nullptr;
    PFNGLUNIFORM2FPROC uniform2f = nullptr;
    PFNGLUNIFORM1FPROC uniform1f = nullptr;

    // 3.0
    PFNGLBINDVERTEXARRAYPROC bindVertexArray = nullptr;
    PFNGLGENVERTEXARRAYSPROC genVertexArrays = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays = nullptr;

    // 3.1
    PFNGLDRAWELEMENTSINSTANCEDPROC drawElementsInstanced = nullptr;

    // 3.3
    PFNGLVERTEXATTRIBDIVISORPROC vertexAttribDivisor = nullptr;

    DispatchTableGL() = default;
    virtual ~DispatchTableGL() = default;

protected:
    virtual void* loadProcAddress(const std::string& function) const = 0;

    void initProcsGL();
};

}
