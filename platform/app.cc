#include "platform/app.h"
#include "base/check.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#include "platform/cocoa/app_mac.h"
#endif

namespace platform {

App::~App() = default;
Window::~Window() = default;

std::unique_ptr<App> App::create(AppOptions options) {
    switch (options.renderer_backend) {
#if BUILDFLAG(IS_MAC)
    case RendererBackend::kOpenGL:
        return AppMac::create(options);
    case RendererBackend::kMetal:
        return nullptr;
#endif
    default:
        NOTREACHED();
    }
}

}  // namespace platform
