#include "experiments/gui_api_redesign/window.h"

struct Window::Impl {};

Window::~Window() = default;
Window::Window() : pimpl_(std::make_unique<Impl>()) {}

std::unique_ptr<Window> Window::create(int width, int height, GLContextManager* mgr) {
    auto window = std::unique_ptr<Window>(new Window());
    return window;
}
