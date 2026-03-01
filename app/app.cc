#include "app/app.h"
#include "app/cocoa/gl/gl_app.h"
#include "base/check.h"
#include "build/build_config.h"

namespace app {

std::unique_ptr<App> create_app(Backend backend) {
    switch (backend) {
#if BUILDFLAG(IS_MAC)
    case Backend::kOpenGL:
        return std::make_unique<GLApp>();
#endif
    default:
        NOTREACHED();
    }
}

}  // namespace app
