#pragma once

#include "gui/key.h"
#include "gui/modifier_key.h"
#include "util/not_copyable_or_movable.h"
#include <memory>

namespace gui {

class App;

class Window {
public:
    NOT_COPYABLE(Window)
    NOT_MOVABLE(Window)
    Window(App& app);
    virtual ~Window();

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
    virtual void onLeftMouseDown(int mouse_x, int mouse_y, gui::ModifierKey modifiers) {}
    virtual void onLeftMouseDrag(int mouse_x, int mouse_y, gui::ModifierKey modifiers) {}
    virtual void onKeyDown(gui::Key key, gui::ModifierKey modifiers) {}
    virtual void onClose() {}
    virtual void onDarkModeToggle() {}

private:
    App& app;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}
