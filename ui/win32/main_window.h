#pragma once

#include "ui/win32/base_window.h"
#include <epoxy/gl.h>
#include <epoxy/wgl.h>

class MainWindow : public BaseWindow<MainWindow> {
public:
    PCWSTR ClassName() const;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HDC ghDC;
    HGLRC ghRC;
};
