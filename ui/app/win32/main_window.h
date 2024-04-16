#pragma once

#include "ui/app/app.h"
#include <windows.h>

class MainWindow {
public:
    HWND m_hwnd;

    MainWindow(App::Window& app_window) : app_window{app_window} {}
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, int wid, HGLRC context);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void redraw();
    BOOL destroy();
    void quit();

private:
    HDC m_hdc;
    HGLRC m_context;
    App::Window& app_window;
};
