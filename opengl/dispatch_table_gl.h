#pragma once

#include "opengl/functionsgl_typedefs.h"
#include "util/non_copyable.h"
#include <string>

namespace opengl {

class DispatchTableGL : util::NonCopyable {
public:
    PFNGLCLEARPROC clear = nullptr;
    PFNGLCLEARCOLORPROC clearColor = nullptr;
    PFNGLDEPTHMASKPROC depthMask = nullptr;
    PFNGLENABLEPROC enable = nullptr;

    DispatchTableGL() = default;
    virtual ~DispatchTableGL() = default;

    void initProcsGL();

protected:
    virtual void* loadProcAddress(const std::string& function) const = 0;
};

}
