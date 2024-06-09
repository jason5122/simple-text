#pragma once

#include "gui/gl/functionsgl_typedefs.h"

namespace gui {

class DispatchTableGL {
public:
    PFNGLCLEARPROC clear = nullptr;
};

}
