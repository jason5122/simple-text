#include "gui/gtk/pimpl_linux.h"
#include "gui/window.h"

namespace gui {

Window::Window(App& app) : pimpl{new impl{app.pimpl->app, this}}, app(app) {}

Window::~Window() {}

void Window::show() {
    pimpl->main_window.show();
}

void Window::close() {
    // pimpl->main_window.close();
}

void Window::redraw() {
    // pimpl->main_window.redraw();
}

int Window::width() {
    // return pimpl->main_window.width();
    return 0;
}

int Window::height() {
    // return pimpl->main_window.height();
    return 0;
}

int Window::scaleFactor() {
    // return pimpl->main_window.scaleFactor();
    return 2;
}

bool Window::isDarkMode() {
    // return pimpl->main_window.isDarkMode();
    return false;
}

void Window::setTitle(const std::string& title) {
    // pimpl->main_window.setTitle(title);
}

void Window::setFilePath(fs::path path) {
    // UNIMPLEMENTED
}

}
