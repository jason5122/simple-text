#include "widget.h"

namespace gui {

renderer::Size Widget::getSize() {
    return size;
}

renderer::Point Widget::getPosition() {
    return position;
}

void Widget::setPosition(const renderer::Point& position) {
    this->position = position;
}

}
