#include "widget.h"

namespace gui {

bool Widget::mousePositionChanged(const std::optional<app::Point>& mouse_pos) {
    return false;
};

app::Size Widget::getSize() const {
    return size;
}

void Widget::setSize(const app::Size& size) {
    this->size = size;
}

void Widget::setWidth(int width) {
    this->size.width = width;
}

void Widget::setHeight(int height) {
    this->size.height = height;
}

app::Point Widget::getPosition() const {
    return position;
}

bool Widget::hitTest(const app::Point& point) {
    return (position.x <= point.x && point.x < position.x + size.width) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

void Widget::setPosition(const app::Point& pos) {
    position = pos;
}

app::CursorStyle Widget::cursorStyle() const {
    return app::CursorStyle::kArrow;
}

Widget* Widget::widgetAt(const app::Point& pos) {
    return hitTest(pos) ? this : nullptr;
}

}  // namespace gui
