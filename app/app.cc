#include "app/app.h"
#include "base/check.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#include "app/cocoa/gl_app.h"
#endif

namespace app {

App::~App() = default;

std::unique_ptr<App> create_app(Backend backend) {
    switch (backend) {
#if BUILDFLAG(IS_MAC)
    case Backend::kOpenGL:
        return GLApp::create();
#endif
    default:
        NOTREACHED();
    }
}

}  // namespace app
