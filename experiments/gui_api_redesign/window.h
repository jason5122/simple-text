#pragma once

#include <functional>
#include <memory>

class Window {
public:
    ~Window();
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void on_draw(std::function<void()> callback) { draw_callback_ = std::move(callback); }
    void invoke_draw_callback() {
        if (draw_callback_) draw_callback_();
    }

private:
    friend class App;
    Window();
    static std::unique_ptr<Window> create(int width, int height);

    int width_;
    int height_;
    std::function<void()> draw_callback_;

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
