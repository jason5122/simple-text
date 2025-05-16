#pragma once

#include <functional>

class Window {
public:
    ~Window();

    void on_draw(std::function<void()> callback) { draw_callback_ = std::move(callback); }
    void invoke_draw_callback() {
        if (draw_callback_) draw_callback_();
    }

private:
    friend class App;
    Window(int width, int height);
    void initialize();

    int width_;
    int height_;
    std::function<void()> draw_callback_;

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
