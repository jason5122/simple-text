#pragma once

#include "ui/app/key.h"
#include "ui/app/modifier_key.h"

class AppWindow {
public:
    virtual void onOpenGLActivate(int width, int height) = 0;
    virtual void onDraw() = 0;
    virtual void onResize(int width, int height) = 0;
    virtual void onScroll(float dx, float dy) = 0;
    virtual void onLeftMouseDown(float mouse_x, float mouse_y) = 0;
    virtual void onLeftMouseDrag(float mouse_x, float mouse_y) = 0;
    virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) = 0;
};
