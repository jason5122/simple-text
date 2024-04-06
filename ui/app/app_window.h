#pragma once

class AppWindow {
public:
    virtual void onOpenGLActivate(int width, int height) = 0;
    virtual void onDraw() = 0;
    virtual void onResize(int width, int height) = 0;
};
