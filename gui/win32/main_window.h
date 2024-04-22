#pragma once

#include "gui/app.h"
#include "gui/win32/dummy_context.h"
#include <windows.h>

class MainWindow {
public:
    HWND m_hwnd;

    MainWindow(App::Window& app_window, DummyContext& dummy_context)
        : app_window{app_window}, dummy_context{dummy_context} {}
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, int wid);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void redraw();
    BOOL destroy();
    void quit();

private:
    HDC m_hdc;
    App::Window& app_window;
    DummyContext& dummy_context;
};
