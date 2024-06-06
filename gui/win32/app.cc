#include "gui/app.h"
#include "gui/win32/pimpl_win.h"
#include <memory>
#include <windows.h>

#include <glad/glad.h>
#include <glad/glad_wgl.h>

namespace gui {

App::App() : pimpl{new impl{}} {}

void App::run() {
    pimpl->dummy_context.initialize();

    this->onLaunch();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void App::quit() {
    PostQuitMessage(0);
}

App::~App() {}

}
