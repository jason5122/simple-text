#include "base/check.h"
#include "gfx/device.h"
#include "gfx/gl/gl_device.h"

namespace gfx {

std::unique_ptr<Device> create_device(Backend backend) {
    switch (backend) {
    case Backend::kOpenGL:
        return std::make_unique<GLDevice>();
    default:
        NOTREACHED();
    }
}

}  // namespace gfx
