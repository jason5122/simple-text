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

    // 1.1
    PFNGLGENTEXTURESPROC genTextures = nullptr;

    DispatchTableGL() = default;
    virtual ~DispatchTableGL() = default;

protected:
    virtual void* loadProcAddress(const std::string& function) const = 0;

    void initProcsGL();
};

}
