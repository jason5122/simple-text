#pragma once

#include "experiments/gui_api_redesign/window.h"
#include <memory>
#include <vector>

class App {
public:
    ~App();
    App();

    bool initialize();
    int run();
    Window& create_window(int width, int height);

private:
    std::vector<std::unique_ptr<Window>> windows_;

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
