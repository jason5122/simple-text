#pragma once

#include "renderer/types.h"

namespace gui {

class Widget {
public:
    Widget(const renderer::Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw(const renderer::Size& screen_size) = 0;
    virtual void scroll(const renderer::Point& delta) {}
    virtual void leftMouseDown(const renderer::Point& mouse) {}
    virtual void leftMouseDrag(const renderer::Point& mouse) {}

    renderer::Size getSize() {
        return size;
    }
    renderer::Point getPosition() {
        return position;
    }
    virtual void setPosition(const renderer::Point& position) {
        this->position = position;
    }

protected:
    renderer::Size size{};
    renderer::Point position{};
};

}
