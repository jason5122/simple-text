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
    Widget(const Size& size) : size_(size) {}
    virtual ~Widget() {}

    // Required overrides.
    virtual void draw() = 0;
    // TODO: Consider changing this to a general-purpose, non-constexpr debug string method.
    virtual constexpr std::string_view class_name() const = 0;

    virtual constexpr void layout() {}
    // TODO: The name `scroll` clashes with some Mac header.
    virtual constexpr void perform_scroll(const Point& mouse_pos, const Delta& delta) {}
    virtual constexpr void left_mouse_down(const Point& mouse_pos,
                                           ModifierKey modifiers,
                                           ClickType click_type) {}
    virtual constexpr void left_mouse_drag(const Point& mouse_pos,
                                           ModifierKey modifiers,
                                           ClickType click_type) {}
    virtual constexpr void left_mouse_up(const Point& mouse_pos) {}
    virtual constexpr void right_mouse_down(const Point& mouse_pos,
                                            ModifierKey modifiers,
                                            ClickType click_type) {}
    virtual constexpr void insert_text(std::string_view str8) {}

    virtual constexpr Widget* widget_at(const Point& pos);
    virtual constexpr bool mouse_position_changed(const std::optional<Point>& mouse_pos);
    virtual constexpr void set_position(const Point& pos);
    virtual constexpr bool can_be_focused() const;
    virtual constexpr CursorStyle cursor_style() const;

    constexpr Size size() const;
    constexpr int width() const;
    constexpr int height() const;
    constexpr void set_size(const Size& size);
    constexpr void set_width(int width);
    constexpr void set_height(int height);
    constexpr Size min_size() const;
    constexpr int min_width() const;
    constexpr int min_height() const;
    constexpr void set_min_size(const Size& min_size);
    constexpr void set_min_width(int min_width);
    constexpr void set_min_height(int min_height);
    constexpr void set_max_size(const Size& max_size);
    constexpr void set_max_width(int max_width);
    constexpr void set_max_height(int max_height);
    constexpr Point position() const;

    constexpr bool is_autoresizing() const;
    constexpr void set_autoresizing(bool autoresizing);
    constexpr bool is_resizable() const;
    constexpr void set_resizable(bool resizable);

    constexpr bool hit_test(const Point& point) const;
    constexpr bool left_edge_test(const Point& point, int distance) const;
    constexpr bool right_edge_test(const Point& point, int distance) const;
    constexpr bool top_edge_test(const Point& point, int distance) const;
    constexpr bool bottom_edge_test(const Point& point, int distance) const;

    // TODO: Refactor singletons.
    inline font::FontRasterizer& rasterizer() {
        return font::FontRasterizer::instance();
    }

private:
    Size size_ = {};
    Size min_size_ = Size::minValue();
    Size max_size_ = Size::maxValue();
    Point position_ = {};
    bool autoresizing_ = true;  // TODO: Consider making this false by default.
    bool resizable_ = true;
};

static_assert(std::is_abstract_v<Widget>);

constexpr Widget* Widget::widget_at(const Point& pos) {
    return hit_test(pos) ? this : nullptr;
}

constexpr bool Widget::mouse_position_changed(const std::optional<Point>& mouse_pos) {
    return false;
}

constexpr void Widget::set_position(const Point& pos) {
    position_ = pos;
}

constexpr bool Widget::can_be_focused() const {
    return false;
}

constexpr CursorStyle Widget::cursor_style() const {
    return CursorStyle::kArrow;
}

constexpr Size Widget::size() const {
    return size_;
}

constexpr int Widget::width() const {
    return size_.width;
}

constexpr int Widget::height() const {
    return size_.height;
}

constexpr void Widget::set_size(const Size& size) {
    set_width(size.width);
    set_height(size.height);
}

constexpr void Widget::set_width(int width) {
    size_.width = std::clamp(width, min_size_.width, max_size_.width);
}

constexpr void Widget::set_height(int height) {
    size_.height = std::clamp(height, min_size_.height, max_size_.height);
}

constexpr Size Widget::min_size() const {
    return min_size_;
}

constexpr int Widget::min_width() const {
    return min_size_.width;
}

constexpr int Widget::min_height() const {
    return min_size_.height;
}

constexpr void Widget::set_min_size(const Size& min_size) {
    this->min_size_ = min_size;
}

constexpr void Widget::set_min_width(int min_width) {
    min_size_.width = min_width;
}

constexpr void Widget::set_min_height(int min_height) {
    min_size_.height = min_height;
}

constexpr void Widget::set_max_size(const Size& max_size) {
    max_size_ = max_size;
}

constexpr void Widget::set_max_width(int max_width) {
    max_size_.width = max_width;
}

constexpr void Widget::set_max_height(int max_height) {
    max_size_.height = max_height;
}

constexpr Point Widget::position() const {
    return position_;
}

constexpr bool Widget::is_autoresizing() const {
    return autoresizing_;
}

constexpr void Widget::set_autoresizing(bool autoresizing) {
    autoresizing_ = autoresizing;
}

constexpr bool Widget::is_resizable() const {
    return resizable_;
}

constexpr void Widget::set_resizable(bool resizable) {
    resizable_ = resizable;
}

constexpr bool Widget::hit_test(const Point& point) const {
    return (position_.x <= point.x && point.x < position_.x + size_.width) &&
           (position_.y <= point.y && point.y < position_.y + size_.height);
}

constexpr bool Widget::left_edge_test(const Point& point, int distance) const {
    int left_offset = position_.x;
    return (std::abs(point.x - left_offset) <= distance) &&
           (position_.y <= point.y && point.y < position_.y + size_.height);
}

constexpr bool Widget::right_edge_test(const Point& point, int distance) const {
    int right_offset = position_.x + size_.width;
    return (std::abs(point.x - right_offset) <= distance) &&
           (position_.y <= point.y && point.y < position_.y + size_.height);
}

constexpr bool Widget::top_edge_test(const Point& point, int distance) const {
    int top_offset = position_.y;
    return (std::abs(point.y - top_offset) <= distance) &&
           (position_.x <= point.x && point.x < position_.x + size_.width);
}

constexpr bool Widget::bottom_edge_test(const Point& point, int distance) const {
    int bottom_offset = position_.y + size_.height;
    return (std::abs(point.y - bottom_offset) <= distance) &&
           (position_.x <= point.x && point.x < position_.x + size_.width);
}

}  // namespace gui
