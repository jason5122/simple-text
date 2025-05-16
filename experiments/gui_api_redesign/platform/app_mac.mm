#include "experiments/gui_api_redesign/app.h"

int App::run() { return 0; }

Window& App::create_window(int width, int height) {
    auto win = std::make_unique<Window>(width, height);
    Window& ref = *win;
    windows_.push_back(std::move(win));
    return ref;
}
