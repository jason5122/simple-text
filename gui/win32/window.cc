#include "gui/win32/pimpl_win.h"
#include "gui/window.h"

namespace gui {

Window::Window(App& app) : pimpl{new impl{*this, app.pimpl->dummy_context}}, app(app) {
    // pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);
}

Window::~Window() {}

void Window::show() {
    pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);

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
    SetWindowPlacement(pimpl->main_window.m_hwnd, &placement);
}

void Window::close() {
    pimpl->main_window.destroy();
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
    return false;
}

}