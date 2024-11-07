#include "app/win32/impl_win.h"
#include "app/window.h"

// TODO: Debug use; remove this.
#include "util/std_print.h"
#include <cassert>

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

int Window::width() const {
    return pimpl->win32_window.width();
}

int Window::height() const {
    return pimpl->win32_window.height();
}

int Window::scaleFactor() const {
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

// TODO: Move this implementation to Win32Window.
std::optional<Point> Window::mousePosition() const {
    POINT mouse_pos;
    GetCursorPos(&mouse_pos);
    ScreenToClient(pimpl->win32_window.m_hwnd, &mouse_pos);

    int window_width = width();
    int window_height = height();

    if ((mouse_pos.x < 0 || mouse_pos.x > window_width - 1) ||
        (mouse_pos.y < 0 || mouse_pos.y > window_height - 1)) {
        return std::nullopt;
    }

    int mouse_x = std::round(mouse_pos.x);
    int mouse_y = std::round(mouse_pos.y);

    assert(!(mouse_x < 0 || mouse_x >= window_width));
    assert(!(mouse_y < 0 || mouse_y >= window_height));

    int scale = scaleFactor();
    int scaled_mouse_x = mouse_x * scale;
    int scaled_mouse_y = mouse_y * scale;
    return Point{scaled_mouse_x, scaled_mouse_y};
}

// TODO: Implement this.
std::optional<Point> Window::mousePositionRaw() const {
    return {};
}

}  // namespace app
