#pragma once

#include <memory>

namespace app {

enum class Backend { kOpenGL, kMetal };

class App {
public:
    virtual ~App() = default;

    virtual void run() = 0;
};

std::unique_ptr<App> create_app(Backend backend);

}  // namespace app
