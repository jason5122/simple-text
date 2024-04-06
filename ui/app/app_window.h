#pragma once

class AppWindow {
public:
    virtual void onOpenGLActivate() = 0;
    virtual void onDraw() = 0;
};
