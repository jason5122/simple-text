#include "ui/app/app.h"
#include "ui/app/win32/main_window.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <memory>
#include <windows.h>

class App::impl {
public:
    int window_count = 0;
};

App::App() : pimpl{new impl{}} {}

void App::run() {
    this->onLaunch();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void App::incrementWindowCount() {
    pimpl->window_count++;
}

App::~App() {}

class App::Window::impl {
public:
    impl(App::Window& app_window) : main_window(app_window) {}

    MainWindow main_window;

    int wid = 0;
};

App::Window::Window(App& parent, int width, int height) : pimpl{new impl{*this}}, parent(parent) {
    pimpl->main_window.create(L"Simple Text", WS_OVERLAPPEDWINDOW, pimpl->wid++);
}

void App::Window::show() {
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
    SetWindowPlacement(pimpl->main_window.hwnd, &placement);
}

void App::Window::close() {
    pimpl->main_window.destroy();

    parent.pimpl->window_count--;
    if (parent.pimpl->window_count <= 0) {
        pimpl->main_window.quit();
    }
}

App::Window::~Window() {}
