#include "widget.h"

namespace gui {

Size Widget::getSize() const {
    return size;
}

void Widget::setSize(const Size& size) {
    this->size = size;
}

void Widget::setWidth(int width) {
    this->size.width = width;
}

void Widget::setHeight(int height) {
    this->size.height = height;
}

Point Widget::getPosition() const {
    return position;
}

bool Widget::hitTest(const Point& point) {
    return (position.x <= point.x && point.x < position.x + size.width) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

void Widget::setPosition(const Point& pos) {
    position = pos;
}

CursorStyle Widget::getCursorStyle() const {
    return CursorStyle::kArrow;
}

}
