#pragma once

#include "experiments/gui_api_redesign/renderer.h"
#include <functional>
#include <memory>

struct WindowCreateInfo;

class Window {
public:
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void on_draw(std::function<void(Renderer&)> callback) { draw_callback_ = std::move(callback); }
    void invoke_draw_callback() {
        if (draw_callback_) draw_callback_(*renderer_);
    }

private:
    friend class App;
    Window();
    static std::unique_ptr<Window> create(WindowCreateInfo info);

    std::function<void(Renderer&)> draw_callback_;
    std::unique_ptr<Renderer> renderer_;

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
