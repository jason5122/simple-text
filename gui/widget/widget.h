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
    virtual constexpr void layout() {}
    virtual constexpr void scroll(const app::Point& mouse_pos, const app::Delta& delta) {}
    virtual constexpr void leftMouseDown(const app::Point& mouse_pos,
                                         app::ModifierKey modifiers,
                                         app::ClickType click_type) {}
    virtual constexpr void leftMouseDrag(const app::Point& mouse_pos,
                                         app::ModifierKey modifiers,
                                         app::ClickType click_type) {}

    virtual constexpr Widget* widgetAt(const app::Point& pos);
    virtual constexpr bool mousePositionChanged(const std::optional<app::Point>& mouse_pos);
    virtual constexpr void setPosition(const app::Point& pos);
    virtual constexpr app::CursorStyle cursorStyle() const;
    // TODO: Debug use; remove this.
    virtual constexpr std::string_view className() const = 0;

    constexpr app::Size getSize() const;
    constexpr int getWidth() const;
    constexpr int getHeight() const;
    constexpr void setSize(const app::Size& size);
    constexpr void setWidth(int width);
    constexpr void setHeight(int height);
    constexpr app::Size getMinimumSize() const;
    constexpr int getMinimumWidth() const;
    constexpr int getMinimumHeight() const;
    constexpr void setMinimumSize(const app::Size& min_size);
    constexpr void setMinimumWidth(int min_width);
    constexpr void setMinimumHeight(int min_height);
    constexpr void setMaximumSize(const app::Size& max_size);
    constexpr void setMaximumWidth(int max_width);
    constexpr void setMaximumHeight(int max_height);
    constexpr app::Point getPosition() const;
    constexpr bool isAutoresizing() const;
    constexpr void setAutoresizing(bool autoresizing);

    constexpr bool hitTest(const app::Point& point) const;
    constexpr bool leftEdgeTest(const app::Point& point, int distance) const;
    constexpr bool rightEdgeTest(const app::Point& point, int distance) const;
    constexpr bool topEdgeTest(const app::Point& point, int distance) const;
    constexpr bool bottomEdgeTest(const app::Point& point, int distance) const;

    // TODO: Refactor singletons.
    inline font::FontRasterizer& rasterizer() {
        return font::FontRasterizer::instance();
    }

protected:
    app::Size size{};
    app::Size min_size = app::Size::minValue();
    app::Size max_size = app::Size::maxValue();
    app::Point position{};
    bool autoresizing = true;  // TODO: Consider making this false by default.
};

static_assert(std::is_abstract_v<Widget>);

constexpr Widget* Widget::widgetAt(const app::Point& pos) {
    return hitTest(pos) ? this : nullptr;
}

constexpr bool Widget::mousePositionChanged(const std::optional<app::Point>& mouse_pos) {
    return false;
}

constexpr void Widget::setPosition(const app::Point& pos) {
    position = pos;
}

constexpr app::CursorStyle Widget::cursorStyle() const {
    return app::CursorStyle::kArrow;
}

constexpr app::Size Widget::getSize() const {
    return size;
}

constexpr int Widget::getWidth() const {
    return size.width;
}

constexpr int Widget::getHeight() const {
    return size.height;
}

constexpr void Widget::setSize(const app::Size& size) {
    setWidth(size.width);
    setHeight(size.height);
}

constexpr void Widget::setWidth(int width) {
    size.width = std::clamp(width, min_size.width, max_size.width);
}

constexpr void Widget::setHeight(int height) {
    size.height = std::clamp(height, min_size.height, max_size.height);
}

constexpr app::Size Widget::getMinimumSize() const {
    return min_size;
}

constexpr int Widget::getMinimumWidth() const {
    return min_size.width;
}

constexpr int Widget::getMinimumHeight() const {
    return min_size.height;
}

constexpr void Widget::setMinimumSize(const app::Size& min_size) {
    this->min_size = min_size;
}

constexpr void Widget::setMinimumWidth(int min_width) {
    min_size.width = min_width;
}

constexpr void Widget::setMinimumHeight(int min_height) {
    min_size.height = min_height;
}

constexpr void Widget::setMaximumSize(const app::Size& max_size) {
    this->max_size = max_size;
}

constexpr void Widget::setMaximumWidth(int max_width) {
    max_size.width = max_width;
}

constexpr void Widget::setMaximumHeight(int max_height) {
    max_size.height = max_height;
}

constexpr app::Point Widget::getPosition() const {
    return position;
}

constexpr bool Widget::isAutoresizing() const {
    return autoresizing;
}

constexpr void Widget::setAutoresizing(bool autoresizing) {
    this->autoresizing = autoresizing;
}

constexpr bool Widget::hitTest(const app::Point& point) const {
    return (position.x <= point.x && point.x < position.x + size.width) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

constexpr bool Widget::leftEdgeTest(const app::Point& point, int distance) const {
    int left_offset = position.x;
    return (std::abs(point.x - left_offset) <= distance) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

constexpr bool Widget::rightEdgeTest(const app::Point& point, int distance) const {
    int right_offset = position.x + size.width;
    return (std::abs(point.x - right_offset) <= distance) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

constexpr bool Widget::topEdgeTest(const app::Point& point, int distance) const {
    int top_offset = position.y;
    return (std::abs(point.y - top_offset) <= distance) &&
           (position.x <= point.x && point.x < position.x + size.width);
}

constexpr bool Widget::bottomEdgeTest(const app::Point& point, int distance) const {
    int bottom_offset = position.y + size.height;
    return (std::abs(point.y - bottom_offset) <= distance) &&
           (position.x <= point.x && point.x < position.x + size.width);
}

}  // namespace gui
