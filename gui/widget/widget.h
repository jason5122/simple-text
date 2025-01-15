#pragma once

#include <algorithm>
#include <optional>

#include "font/font_rasterizer.h"
#include "gui/platform/key.h"
#include "gui/types.h"

namespace gui {

class Widget {
public:
    Widget() {}
    Widget(const Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw() = 0;
    virtual constexpr void layout() {}
    // TODO: The name `scroll` clashes with some Mac header.
    virtual constexpr void performScroll(const Point& mouse_pos, const Delta& delta) {}
    virtual constexpr void leftMouseDown(const Point& mouse_pos,
                                         ModifierKey modifiers,
                                         ClickType click_type) {}
    virtual constexpr void leftMouseDrag(const Point& mouse_pos,
                                         ModifierKey modifiers,
                                         ClickType click_type) {}
    virtual constexpr void leftMouseUp(const Point& mouse_pos) {}
    virtual constexpr void rightMouseDown(const Point& mouse_pos,
                                          ModifierKey modifiers,
                                          ClickType click_type) {}
    virtual constexpr void insertText(std::string_view str8) {}

    virtual constexpr Widget* widgetAt(const Point& pos);
    virtual constexpr bool mousePositionChanged(const std::optional<Point>& mouse_pos);
    virtual constexpr void setPosition(const Point& pos);
    virtual constexpr bool canBeFocused() const;
    virtual constexpr CursorStyle cursorStyle() const;
    // TODO: Debug use; remove this.
    virtual constexpr std::string_view className() const = 0;

    constexpr Size getSize() const;
    constexpr int getWidth() const;
    constexpr int getHeight() const;
    constexpr void setSize(const Size& size);
    constexpr void setWidth(int width);
    constexpr void setHeight(int height);
    constexpr Size getMinimumSize() const;
    constexpr int getMinimumWidth() const;
    constexpr int getMinimumHeight() const;
    constexpr void setMinimumSize(const Size& min_size);
    constexpr void setMinimumWidth(int min_width);
    constexpr void setMinimumHeight(int min_height);
    constexpr void setMaximumSize(const Size& max_size);
    constexpr void setMaximumWidth(int max_width);
    constexpr void setMaximumHeight(int max_height);
    constexpr Point getPosition() const;

    constexpr bool isAutoresizing() const;
    constexpr void setAutoresizing(bool autoresizing);
    constexpr bool isResizable() const;
    constexpr void setResizable(bool resizable);

    constexpr bool hitTest(const Point& point) const;
    constexpr bool leftEdgeTest(const Point& point, int distance) const;
    constexpr bool rightEdgeTest(const Point& point, int distance) const;
    constexpr bool topEdgeTest(const Point& point, int distance) const;
    constexpr bool bottomEdgeTest(const Point& point, int distance) const;

    // TODO: Refactor singletons.
    inline font::FontRasterizer& rasterizer() {
        return font::FontRasterizer::instance();
    }

protected:
    Size size{};
    Size min_size = Size::minValue();
    Size max_size = Size::maxValue();
    Point position{};
    bool autoresizing = true;  // TODO: Consider making this false by default.
    bool resizable = true;
};

static_assert(std::is_abstract_v<Widget>);

constexpr Widget* Widget::widgetAt(const Point& pos) {
    return hitTest(pos) ? this : nullptr;
}

constexpr bool Widget::mousePositionChanged(const std::optional<Point>& mouse_pos) {
    return false;
}

constexpr void Widget::setPosition(const Point& pos) {
    position = pos;
}

constexpr bool Widget::canBeFocused() const {
    return false;
}

constexpr CursorStyle Widget::cursorStyle() const {
    return CursorStyle::kArrow;
}

constexpr Size Widget::getSize() const {
    return size;
}

constexpr int Widget::getWidth() const {
    return size.width;
}

constexpr int Widget::getHeight() const {
    return size.height;
}

constexpr void Widget::setSize(const Size& size) {
    setWidth(size.width);
    setHeight(size.height);
}

constexpr void Widget::setWidth(int width) {
    size.width = std::clamp(width, min_size.width, max_size.width);
}

constexpr void Widget::setHeight(int height) {
    size.height = std::clamp(height, min_size.height, max_size.height);
}

constexpr Size Widget::getMinimumSize() const {
    return min_size;
}

constexpr int Widget::getMinimumWidth() const {
    return min_size.width;
}

constexpr int Widget::getMinimumHeight() const {
    return min_size.height;
}

constexpr void Widget::setMinimumSize(const Size& min_size) {
    this->min_size = min_size;
}

constexpr void Widget::setMinimumWidth(int min_width) {
    min_size.width = min_width;
}

constexpr void Widget::setMinimumHeight(int min_height) {
    min_size.height = min_height;
}

constexpr void Widget::setMaximumSize(const Size& max_size) {
    this->max_size = max_size;
}

constexpr void Widget::setMaximumWidth(int max_width) {
    max_size.width = max_width;
}

constexpr void Widget::setMaximumHeight(int max_height) {
    max_size.height = max_height;
}

constexpr Point Widget::getPosition() const {
    return position;
}

constexpr bool Widget::isAutoresizing() const {
    return autoresizing;
}

constexpr void Widget::setAutoresizing(bool autoresizing) {
    this->autoresizing = autoresizing;
}

constexpr bool Widget::isResizable() const {
    return resizable;
}

constexpr void Widget::setResizable(bool resizable) {
    this->resizable = resizable;
}

constexpr bool Widget::hitTest(const Point& point) const {
    return (position.x <= point.x && point.x < position.x + size.width) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

constexpr bool Widget::leftEdgeTest(const Point& point, int distance) const {
    int left_offset = position.x;
    return (std::abs(point.x - left_offset) <= distance) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

constexpr bool Widget::rightEdgeTest(const Point& point, int distance) const {
    int right_offset = position.x + size.width;
    return (std::abs(point.x - right_offset) <= distance) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

constexpr bool Widget::topEdgeTest(const Point& point, int distance) const {
    int top_offset = position.y;
    return (std::abs(point.y - top_offset) <= distance) &&
           (position.x <= point.x && point.x < position.x + size.width);
}

constexpr bool Widget::bottomEdgeTest(const Point& point, int distance) const {
    int bottom_offset = position.y + size.height;
    return (std::abs(point.y - bottom_offset) <= distance) &&
           (position.x <= point.x && point.x < position.x + size.width);
}

}  // namespace gui
