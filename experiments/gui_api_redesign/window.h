#pragma once

#include <functional>

class Window {
public:
    Window(int width, int height);
    ~Window();

    void on_draw(std::function<void()> callback) { draw_callback = std::move(callback); }
    void invoke_draw_callback() {
        if (draw_callback) draw_callback();
    }

private:
    int width_;
    int height_;
    std::function<void()> draw_callback;

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
