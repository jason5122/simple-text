#include "ui/app/app.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <vector>
#include <windows.h>
#include <windowsx.h>

class App::impl {
public:
    // std::vector<HWND> hwnds;
};

App::App() : pimpl{new impl{}} {
    LPCTSTR cursor = IDC_IBEAM;
    HCURSOR hCursor = LoadCursor(NULL, cursor);
    SetCursor(hCursor);
}

void App::run() {
    this->onActivate();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void App::createNewWindow() {}

App::~App() {}
