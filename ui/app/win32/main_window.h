#pragma once

#include "ui/app/win32/base_window.h"
#include "util/profile_util.h"
#include <glad/glad.h>
#include <glad/glad_wgl.h>
#include <windows.h>

#define ID_QUIT 0x70

class MainWindow : public BaseWindow<MainWindow> {
public:
    MainWindow() {}

    PCWSTR className() const {
        return L"Clock Window Class";
    }
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HDC ghDC;
    HGLRC ghRC;
};
