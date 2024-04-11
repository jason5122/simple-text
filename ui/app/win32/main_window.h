#pragma once

#include "ui/app/app.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <windows.h>

class MainWindow {
public:
    HWND hwnd;

    MainWindow(App::Window& app_window) : app_window{app_window} {}
    BOOL create(PCWSTR lpWindowName, DWORD dwStyle, int wid);
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void redraw();
    BOOL destroy();
    void quit();

private:
    HDC ghDC;
    HGLRC ghRC;
    App::Window& app_window;
};
