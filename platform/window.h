#pragma once

#include "gfx/frame.h"
#include <string>
#include <string_view>
#include <variant>

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

struct DrawEvent {
    gfx::Frame& frame;
    FrameInfo frame_info;
};

struct ResizeEvent {
    int width_px = 0;
    int height_px = 0;
    float scale_factor = 1.0f;
};

struct ScrollEvent {
    float dx = 0;
    float dy = 0;
};

struct CloseRequestEvent {
    bool* allow_close = nullptr;
};

struct CloseEvent {};

using Event = std::variant<DrawEvent, ResizeEvent, ScrollEvent, CloseRequestEvent, CloseEvent>;

class Window;

class WindowDelegate {
public:
    virtual ~WindowDelegate() = default;
    virtual void on_event(Window& window, const Event& event) = 0;
};

class Window {
public:
    virtual ~Window();

    virtual void set_title(std::string_view title) = 0;
    virtual void request_redraw() = 0;
    virtual void set_continuous_redraw(bool enabled) = 0;
};

}  // namespace platform
