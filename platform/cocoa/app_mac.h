#pragma once

#include "platform/app.h"
#include <memory>
#include <vector>

namespace platform {

class WindowMac;

class AppMac final : public App {
public:
    static std::unique_ptr<App> create(AppOptions options);
    ~AppMac() override;

    Window* create_window(WindowOptions options, WindowDelegate* delegate) override;
    int run() override;
    void quit() override;

    void window_did_close(WindowMac* window);

private:
    explicit AppMac(AppOptions options);

    AppOptions options_;
    std::vector<std::unique_ptr<WindowMac>> windows_;
    void* app_delegate_ = nullptr;
};

}  // namespace platform
