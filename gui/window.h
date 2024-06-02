#pragma once

#include "gui/action.h"
#include "gui/key.h"
#include "gui/modifier_key.h"
#include "util/not_copyable_or_movable.h"
#include <filesystem>
#include <memory>
#include <string_view>

namespace fs = std::filesystem;

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
    void setTitle(const std::string& title);
    void setFilePath(fs::path path);

    virtual void onOpenGLActivate(int width, int height) {}
    virtual void onDraw() {}
    virtual void onResize(int width, int height) {}
    virtual void onScroll(int dx, int dy) {}
    virtual void onLeftMouseDown(int mouse_x, int mouse_y, gui::ModifierKey modifiers) {}
    virtual void onLeftMouseDrag(int mouse_x, int mouse_y, gui::ModifierKey modifiers) {}
    virtual bool onKeyDown(gui::Key key, gui::ModifierKey modifiers) {
        return false;
    }
    virtual void onInsertText(std::string_view text) {}
    virtual void onAction(gui::Action action) {}
    virtual void onClose() {}
    virtual void onDarkModeToggle() {}

private:
    App& app;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}
