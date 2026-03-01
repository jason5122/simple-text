#include "base/check.h"
#include "build/build_config.h"
#include "gfx/device.h"
#include "gfx/gl/gl_device.h"

namespace gfx {

std::unique_ptr<Device> create_device(Backend backend) {
    switch (backend) {
#if BUILDFLAG(IS_MAC)
    case Backend::kOpenGL:
        return std::make_unique<GLDevice>();
#endif
    default:
        NOTREACHED();
    }
}

}  // namespace gfx
