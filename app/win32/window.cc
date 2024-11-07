#include "app/win32/impl_win.h"
#include "app/window.h"

namespace app {

Window::Window(App& app, int width, int height)
    : pimpl{new impl{*this, app.pimpl->dummy_context}} {}

Window::~Window() {}

void Window::show() {
    pimpl->win32_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);

    // TODO: Sync this with requested width/height.
    int width = 1200;
    int height = 600;

    // FIXME: This doesn't animate like ShowWindow().
    WINDOWPLACEMENT placement{
        .length = sizeof(WINDOWPLACEMENT),
        // .showCmd = SW_MAXIMIZE,
        .showCmd = SW_NORMAL,
        .rcNormalPosition = RECT{0, 0, width * 2, height * 2},
    };
    SetWindowPlacement(pimpl->win32_window.m_hwnd, &placement);
}

void Window::close() {
    pimpl->win32_window.destroy();
}

void Window::redraw() {
    pimpl->win32_window.redraw();
}

int Window::width() {
    return pimpl->win32_window.width();
}

int Window::height() {
    return pimpl->win32_window.height();
}

int Window::scaleFactor() {
    return pimpl->win32_window.scaleFactor();
}

bool Window::isDarkMode() {
    return false;
}

void Window::setTitle(const std::string& title) {
    pimpl->win32_window.setTitle(title);
}

// TODO: Implement this.
void Window::setFilePath(fs::path path) {}

// TODO: Implement this.
std::optional<std::string> Window::openFilePicker() const {
    return {};
}

// TODO: Implement this.
std::optional<app::Point> Window::mousePosition() const {
    return {};
}

// TODO: Implement this.
std::optional<app::Point> Window::mousePositionRaw() const {
    return {};
}

}  // namespace app
