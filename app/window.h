#pragma once

#include "app/action.h"
#include "app/app_action.h"
#include "app/key.h"
#include "app/modifier_key.h"
#include "util/non_copyable.h"
#include <filesystem>
#include <memory>
#include <string_view>

namespace fs = std::filesystem;

namespace app {

enum class ClickType {
    kSingleClick,
    kDoubleClick,
    kTripleClick,
};

class App;

class Window {
public:
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
    virtual void onDraw(int width, int height) {}
    virtual void onResize(int width, int height) {}
    virtual void onScroll(int dx, int dy) {}
    virtual void onLeftMouseDown(int mouse_x, int mouse_y, ModifierKey modifiers,
                                 ClickType click_type) {}
    virtual void onLeftMouseDrag(int mouse_x, int mouse_y, ModifierKey modifiers) {}
    virtual bool onKeyDown(Key key, ModifierKey modifiers) {
        return false;
    }
    virtual void onInsertText(std::string_view text) {}
    virtual void onAction(Action action) {}
    virtual void onClose() {}
    virtual void onDarkModeToggle() {}
    virtual void onAppAction(AppAction action) {}

private:
    App& app;

    class impl;
    std::unique_ptr<impl> pimpl;
};

}
