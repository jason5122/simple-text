#include "widget.h"

namespace gui {

renderer::Size Widget::getSize() {
    return size;
}

void Widget::setWidth(int width) {
    this->size.width = width;
}

void Widget::setHeight(int height) {
    this->size.height = height;
}

renderer::Point Widget::getPosition() {
    return position;
}

void Widget::setPosition(const renderer::Point& position) {
    this->position = position;
}

bool Widget::hitTest(const renderer::Point& point) {
    return (position.x <= point.x && point.x < position.x + size.width) &&
           (position.y <= point.y && point.y < position.y + size.height);
}

}
