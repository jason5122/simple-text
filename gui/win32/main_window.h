#pragma once

#include "gui/win32/dummy_context.h"
#include "gui/window.h"
#include <windows.h>

namespace gui {

class MainWindow {
public:
    HWND m_hwnd;

    MainWindow(Window& app_window, DummyContext& dummy_context)
        : app_window{app_window}, dummy_context{dummy_context} {}
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, int wid);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void redraw();
    BOOL destroy();
    void quit();
    int width();
    int height();
    int scaleFactor();

private:
    HDC m_hdc;
    Window& app_window;
    DummyContext& dummy_context;

    WCHAR high_surrogate = '\0';
};

}
