#pragma once

#include "app/action.h"
#include "app/app_action.h"
#include "app/key.h"
#include "app/modifier_key.h"
#include "app/types.h"
#include "util/non_copyable.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace app {

enum class ClickType {
    kSingleClick,
    kDoubleClick,
    kTripleClick,
};

constexpr ClickType ClickTypeFromCount(int click_count) {
    if (click_count == 1) {
        return ClickType::kSingleClick;
    } else if (click_count == 2) {
        return ClickType::kDoubleClick;
    } else {
        return ClickType::kTripleClick;
    }
}

class App;

class Window {
public:
    Window(App& app, int width, int height);
    virtual ~Window();

    void show();
    void close();
    void redraw();
    int width() const;
    int height() const;
    int scaleFactor() const;
    bool isDarkMode();
    void setTitle(const std::string& title);
    void setFilePath(fs::path path);
    std::optional<std::string> openFilePicker() const;
    std::optional<app::Point> mousePosition() const;
    // TODO: Rename this to `mousePosition()` and the original `mousePosition()` to
    // `mousePositionScaled()`.
    std::optional<app::Point> mousePositionRaw() const;
    void createMenuDebug() const;

    virtual void onOpenGLActivate(int width, int height) {}
    virtual void onDraw(int width, int height) {}
    virtual void onResize(int width, int height) {}
    virtual void onScroll(int mouse_x, int mouse_y, int dx, int dy) {}
    virtual void onLeftMouseDown(int mouse_x,
                                 int mouse_y,
                                 ModifierKey modifiers,
                                 ClickType click_type) {}
    virtual void onLeftMouseUp() {}
    virtual void onLeftMouseDrag(int mouse_x,
                                 int mouse_y,
                                 ModifierKey modifiers,
                                 ClickType click_type) {}
    virtual void onRightMouseDown(int mouse_x,
                                  int mouse_y,
                                  ModifierKey modifiers,
                                  ClickType click_type) {}
    virtual void onMouseMove() {}
    virtual void onMouseExit() {}
    virtual bool onKeyDown(Key key, ModifierKey modifiers) {
        return false;
    }
    virtual void onInsertText(std::string_view text) {}
    virtual void onAction(Action action, bool extend = false) {}
    virtual void onAppAction(AppAction action) {}
    virtual void onClose() {}
    virtual void onDarkModeToggle() {}

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace app
