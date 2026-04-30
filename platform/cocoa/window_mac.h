#pragma once

#include "platform/window.h"
#include "platform/task.h"
#include <corral/corral.h>
#include <functional>
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
    TaskScope task_scope() const override;
    void* native_handle() const override;
    void request_redraw() override;
    void set_continuous_redraw(bool enabled) override;

    bool should_close();
    void did_close();
    corral::Task<void> run_until_closed();
    void emit_resize(const ResizeInfo& resize_info);
    void emit_scroll(const ScrollInfo& scroll_info);
    void emit_pointer_move(const PointerInfo& pointer_info);
    void emit_pointer_down(const PointerInfo& pointer_info);
    void emit_pointer_up(const PointerInfo& pointer_info);
    void emit_draw(gfx::Frame& frame, const FrameInfo& frame_info);
    AppMac& app();

private:
    WindowMac(AppMac& app, WindowOptions options, WindowDelegate* delegate);

    AppMac& app_;
    WindowDelegate* delegate_ = nullptr;
    std::shared_ptr<internal::TaskScopeState> task_scope_;
    bool close_requested_ = false;
    std::function<void()> emit_close_;
    NSWindow* ns_window_ = nullptr;
    PlatformGLView* gl_view_ = nullptr;
    PlatformWindowDelegate* window_delegate_ = nullptr;
};

}  // namespace platform
