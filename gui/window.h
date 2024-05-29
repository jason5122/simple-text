#pragma once

#include "gui/key.h"
#include "gui/modifier_key.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

class App;

class Window2 {
public:
    NOT_COPYABLE(Window2)
    NOT_MOVABLE(Window2)
    Window2(App& app);
    virtual ~Window2();

    void show();
    void close();
    void redraw();
    int width();
    int height();
    int scaleFactor();
    bool isDarkMode();

    virtual void onOpenGLActivate(int width, int height) {}
    virtual void onDraw() {}
    virtual void onResize(int width, int height) {}
    virtual void onScroll(int dx, int dy) {}
    virtual void onLeftMouseDown(int mouse_x, int mouse_y, app::ModifierKey modifiers) {}
    virtual void onLeftMouseDrag(int mouse_x, int mouse_y, app::ModifierKey modifiers) {}
    virtual void onKeyDown(app::Key key, app::ModifierKey modifiers) {}
    virtual void onClose() {}
    virtual void onDarkModeToggle() {}

private:
    friend class App;
    App& app;

    class impl;
    std::unique_ptr<impl> pimpl;
};
