#pragma once

#include <functional>

class Window {
public:
    Window(int width, int height);
    ~Window();

    void on_draw(std::function<void()> callback);

private:
    int width_;
    int height_;

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
