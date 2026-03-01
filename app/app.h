#pragma once

#include "app/window.h"
#include <memory>

namespace app {

enum class Backend { kOpenGL, kMetal };

class App {
public:
    virtual ~App();

    virtual void run() = 0;
    virtual std::unique_ptr<Window> create_window(const WindowOptions& options) = 0;
};

std::unique_ptr<App> create_app(Backend backend);

}  // namespace app
