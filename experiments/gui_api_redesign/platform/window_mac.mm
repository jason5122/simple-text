#include "experiments/gui_api_redesign/window.h"

struct Window::Impl {};

Window::Window(int width, int height) : width_(width), height_(height) {}

Window::~Window() = default;

void Window::on_draw(std::function<void()> callback) {}
