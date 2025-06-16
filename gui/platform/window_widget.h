#pragma once

#include "gui/platform/action.h"
#include "gui/platform/key.h"
#include "gui/types.h"
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
    bool is_dark_mode() const;
    void set_title(std::string_view title);
    void set_file_path(std::string_view path);
    std::optional<std::string> open_file_picker() const;
    void set_cursor_style(CursorStyle style);
    void set_auto_redraw(bool auto_redraw);
    int frames_per_second() const;

    void create_menu_debug() const;

    virtual void on_opengl_activate() {}
    virtual void on_frame(int64_t ms) {}
    virtual void on_scroll_decelerate(const Point& mouse_pos, const Delta& delta) {}
    virtual bool on_key_down(Key key, ModifierKey modifiers) { return false; }
    virtual void on_insert_text(std::string_view text) {}
    virtual void on_action(Action action, bool extend = false) {}
    virtual void on_app_action(AppAction action) {}
    virtual void on_close() {}
    virtual void on_dark_mode_toggle() {}

    // TODO: This is public for callbacks. Find a way to make this private.
    // private:
    class Impl;
    std::unique_ptr<Impl> pimpl;

private:
    CursorStyle current_style = CursorStyle::kArrow;
};

}  // namespace gui
