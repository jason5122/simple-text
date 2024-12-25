#pragma once

#include "app/modifier_key.h"
#include "app/types.h"
#include "app/window.h"
#include "font/font_rasterizer.h"

namespace gui {

class Widget {
public:
    Widget() {}
    Widget(const app::Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw() = 0;
    virtual void scroll(const app::Point& mouse_pos, const app::Delta& delta) {}
    virtual void leftMouseDown(const app::Point& mouse_pos,
                               app::ModifierKey modifiers,
                               app::ClickType click_type) {}
    virtual void leftMouseDrag(const app::Point& mouse_pos,
                               app::ModifierKey modifiers,
                               app::ClickType click_type) {}
    virtual bool mousePositionChanged(const std::optional<app::Point>& mouse_pos);
    virtual void layout() {}
    virtual void setPosition(const app::Point& pos);
    virtual app::CursorStyle cursorStyle() const;
    virtual Widget* widgetAt(const app::Point& pos);

    app::Size getSize() const;
    void setSize(const app::Size& size);
    void setWidth(int width);
    void setHeight(int height);
    app::Point getPosition() const;
    bool hitTest(const app::Point& point);
    bool isAutoresizing() const;
    void setAutoresizing(bool autoresizing);

    // TODO: Debug use; remove this.
    virtual std::string_view className() const = 0;

    // TODO: Refactor singletons.
    inline font::FontRasterizer& rasterizer() {
        return font::FontRasterizer::instance();
    }

protected:
    app::Size size{};
    app::Point position{};
    bool autoresizing = true;  // TODO: Consider making this false by default.
};

static_assert(std::is_abstract<Widget>());

}  // namespace gui
