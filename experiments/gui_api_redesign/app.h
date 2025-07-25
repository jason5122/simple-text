#pragma once

#include "experiments/gui_api_redesign/window.h"
#include <memory>
#include <vector>

class App {
public:
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    static std::unique_ptr<App> create();

    int run();
    Window* create_window(int width, int height);

private:
    App();

    std::vector<std::unique_ptr<Window>> windows_;

    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};
