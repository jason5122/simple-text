#pragma once

#include "gui/app/action.h"
#include "gui/app/app_action.h"
#include "gui/app/key.h"
#include "gui/app/modifier_key.h"
#include "gui/app/types.h"
#include "gui/widget/widget.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace gui {

class App;

class WindowWidget : public Widget {
public:
    WindowWidget(App& app, int width, int height);
    virtual ~WindowWidget();

    void show();
    void close();
    void redraw();
    int scale() const;
    bool isDarkMode() const;
    void setTitle(std::string_view title);
    void setFilePath(std::string_view path);
    std::optional<std::string> openFilePicker() const;
    void setCursorStyle(CursorStyle style);
    void setAutoRedraw(bool auto_redraw);
    int framesPerSecond() const;

    void createMenuDebug() const;

    virtual void onOpenGLActivate() {}
    virtual void onFrame(int64_t ms) {}
    virtual void onScrollDecelerate(const Point& mouse_pos, const Delta& delta) {}
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

}  // namespace gui
