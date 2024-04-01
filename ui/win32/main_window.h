#pragma once

#include "ui/renderer/image_renderer.h"
#include "ui/renderer/rect_renderer.h"
#include "ui/win32/base_window.h"

class MainWindow : public BaseWindow<MainWindow> {
public:
    PCWSTR ClassName() const;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    HDC ghDC;
    HGLRC ghRC;
    ImageRenderer image_renderer;
    RectRenderer rect_renderer;
};
