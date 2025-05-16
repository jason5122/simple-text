#pragma once

#include <memory>
#include <vector>

#include "experiments/gui_api_redesign/window.h"

class App {
public:
    int run();
    Window& create_window(int width, int height);

private:
    std::vector<std::unique_ptr<Window>> windows_;
};
