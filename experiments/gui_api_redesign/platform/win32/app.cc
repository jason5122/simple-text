#include "experiments/gui_api_redesign/app.h"

struct App::Impl {};

App::~App() = default;
App::App() : pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<App> App::create() {
    auto app = std::unique_ptr<App>(new App());
    return app;
}

int App::run() { return 0; }

Window& App::create_window(int width, int height) {
    auto window = Window::create(width, height, {});
    Window& ref = *window;
    windows_.push_back(std::move(window));
    return ref;
}
