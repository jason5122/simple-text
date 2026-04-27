#pragma once

#include "platform/window.h"
#include <memory>

namespace platform {

enum class RendererBackend { kOpenGL, kMetal };

struct AppOptions {
    RendererBackend renderer_backend = RendererBackend::kOpenGL;
};

class App {
public:
    virtual ~App();

    static std::unique_ptr<App> create(AppOptions options = {});

    virtual Window* create_window(WindowOptions options, WindowDelegate* delegate) = 0;
    virtual int run() = 0;
    virtual void quit() = 0;
};

}  // namespace platform
