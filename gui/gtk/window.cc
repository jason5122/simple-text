#include "gui/gtk/pimpl_linux.h"
#include "gui/window.h"

namespace gui {

Window::Window(App& app) : pimpl{new impl{app.pimpl->app, this}}, app(app) {
    // MainWindow main_window1{app.pimpl->app, this};
    // MainWindow main_window2{app.pimpl->app, this};
}

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
    return pimpl->main_window.isDarkMode();
}

void Window::setTitle(const std::string& title) {
    pimpl->main_window.setTitle(title);
}

void Window::setFilePath(fs::path path) {
    // UNIMPLEMENTED
}

}
