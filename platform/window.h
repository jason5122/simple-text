#pragma once

#include "gfx/frame.h"
#include <string>
#include <string_view>

namespace platform {

struct WindowOptions {
    int width = 1200;
    int height = 800;
    std::string title;
};

struct FrameInfo {
    double time_seconds = 0;
    int width_px = 0;
    int height_px = 0;
    float scale_factor = 1.0f;
};

struct ResizeInfo {
    int width_px = 0;
    int height_px = 0;
    float scale_factor = 1.0f;
};

struct ScrollInfo {
    float dx = 0;
    float dy = 0;
};

struct PointerInfo {
    float x_px = 0;
    float y_px = 0;
    int button = 0;
};

class Window;

class WindowDelegate {
public:
    virtual ~WindowDelegate() = default;

    virtual void on_draw(Window& window, gfx::Frame& frame, const FrameInfo& frame_info) {}
    virtual void on_resize(Window& window, const ResizeInfo& resize_info) {}
    virtual void on_scroll(Window& window, const ScrollInfo& scroll_info) {}
    virtual void on_pointer_move(Window& window, const PointerInfo& pointer_info) {}
    virtual void on_pointer_down(Window& window, const PointerInfo& pointer_info) {}
    virtual void on_pointer_up(Window& window, const PointerInfo& pointer_info) {}
    virtual bool on_close_request(Window& window) { return true; }
    virtual void on_close(Window& window) {}
};

class Window {
public:
    virtual ~Window();

    virtual void set_title(std::string_view title) = 0;
    virtual void request_redraw() = 0;
    virtual void set_continuous_redraw(bool enabled) = 0;
};

}  // namespace platform
