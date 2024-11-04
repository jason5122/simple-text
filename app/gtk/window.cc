#include "app/gtk/impl_gtk.h"
#include "app/window.h"

namespace app {

Window::Window(App& app, int width, int height)
    : pimpl{new impl{app.pimpl->app, this, app.pimpl->context}} {}

Window::~Window() {}

void Window::show() {
    pimpl->main_window.show();
}

void Window::close() {
    pimpl->main_window.close();
}

void Window::redraw() {
    pimpl->main_window.redraw();
}

int Window::width() {
    return pimpl->main_window.width();
}

int Window::height() {
    return pimpl->main_window.height();
}

int Window::scaleFactor() {
    return pimpl->main_window.scaleFactor();
}

bool Window::isDarkMode() {
    // return pimpl->main_window.isDarkMode();
    return false;
}

void Window::setTitle(const std::string& title) {
    pimpl->main_window.setTitle(title);
}

// TODO: Implement this.
void Window::setFilePath(fs::path path) {}

// TODO: Implement this.
std::optional<std::string> Window::openFilePicker() {
    return {};
}

// TODO: Implement this.
std::optional<app::Point> Window::mousePosition() {
    return {};
}

// TODO: Implement this.
std::optional<app::Point> Window::mousePositionRaw() {
    return {};
}

}  // namespace app
