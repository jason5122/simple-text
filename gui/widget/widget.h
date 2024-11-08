#pragma once

#include "app/modifier_key.h"
#include "app/window.h"
#include "font/font_rasterizer.h"
#include "gui/renderer/types.h"

namespace gui {

class Widget {
public:
    Widget() {}
    Widget(const Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw(const std::optional<app::Point>& mouse_pos) = 0;
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
    virtual app::CursorStyle getCursorStyle() const;
    virtual Widget* getWidgetAtPosition(const app::Point& pos);

    Size getSize() const;
    void setSize(const Size& size);
    void setWidth(int width);
    void setHeight(int width);
    app::Point getPosition() const;
    bool hitTest(const app::Point& point);

    // TODO: Debug use; remove this.
    virtual std::string_view getClassName() const = 0;

    // TODO: Refactor singletons.
    inline font::FontRasterizer& rasterizer() {
        return font::FontRasterizer::instance();
    };

protected:
    Size size{};
    app::Point position{};
};

static_assert(std::is_abstract<Widget>());

}  // namespace gui

template <>
struct std::formatter<gui::Widget> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& widget, auto& ctx) const {
        return std::format_to(ctx.out(), "{}({})", widget.getClassName(), widget.getSize());
    }
};
