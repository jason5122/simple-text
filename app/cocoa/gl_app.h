#pragma once

#include "app/app.h"

namespace app {

class GLApp final : public App {
public:
    static std::unique_ptr<App> create();

    void run() override;
    std::unique_ptr<Window> create_window(const WindowOptions& options) override;

private:
    GLApp();
};

}  // namespace app
