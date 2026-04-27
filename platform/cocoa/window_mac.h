#pragma once

#include "platform/window.h"
#include <memory>

@class NSWindow;
@class PlatformGLView;
@class PlatformWindowDelegate;

namespace platform {

class AppMac;

class WindowMac final : public Window {
public:
    static std::unique_ptr<WindowMac> create(AppMac& app,
                                             WindowOptions options,
                                             WindowDelegate* delegate);
    ~WindowMac() override;

    void set_title(std::string_view title) override;
    void request_redraw() override;
    void set_continuous_redraw(bool enabled) override;

    bool should_close();
    void did_close();
    void emit_event(const Event& event);
    AppMac& app();

private:
    WindowMac(AppMac& app, WindowOptions options, WindowDelegate* delegate);

    AppMac& app_;
    WindowDelegate* delegate_ = nullptr;
    NSWindow* ns_window_ = nullptr;
    PlatformGLView* gl_view_ = nullptr;
    PlatformWindowDelegate* window_delegate_ = nullptr;
};

}  // namespace platform
