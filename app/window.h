#pragma once

#include "app/action.h"
#include "app/app_action.h"
#include "app/key.h"
#include "app/modifier_key.h"
#include "app/types.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

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
    int scale() const;
    bool isDarkMode() const;
    void setTitle(std::string_view title);
    void setFilePath(std::string_view path);
    std::optional<std::string> openFilePicker() const;
    void setCursorStyle(CursorStyle style);
    void setAutoRedraw(bool auto_redraw);
    int framesPerSecond() const;

    void createMenuDebug() const;

    virtual void onOpenGLActivate(const Size& size) {}
    virtual void onDraw(const Size& size) {}
    virtual void onFrame(int64_t ms) {}
    virtual void onResize(const Size& size) {}
    virtual void onScroll(const Point& mouse_pos, const Delta& delta) {}
    virtual void onScrollDecelerate(const Point& mouse_pos, const Delta& delta) {}
    virtual void onLeftMouseDown(const Point& mouse_pos,
                                 ModifierKey modifiers,
                                 ClickType click_type) {}
    virtual void onLeftMouseUp(const app::Point& mouse_pos) {}
    virtual void onLeftMouseDrag(const Point& mouse_pos,
                                 ModifierKey modifiers,
                                 ClickType click_type) {}
    virtual void onRightMouseDown(const Point& mouse_pos,
                                  ModifierKey modifiers,
                                  ClickType click_type) {}
    virtual void onMouseMove(const Point& mouse_pos) {}
    virtual void onMouseExit() {}
    virtual bool onKeyDown(Key key, ModifierKey modifiers) {
        return false;
    }
    virtual void onInsertText(std::string_view text) {}
    virtual void onAction(Action action, bool extend = false) {}
    virtual void onAppAction(AppAction action) {}
    virtual void onClose() {}
    virtual void onDarkModeToggle() {}

    // TODO: This is public for callbacks. Find a way to make this private.
    // private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace app
