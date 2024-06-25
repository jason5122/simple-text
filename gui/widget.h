#pragma once

#include "renderer/types.h"

namespace gui {

class Widget {
public:
    Widget() {}
    Widget(const renderer::Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw() = 0;
    virtual void scroll(const renderer::Point& delta) {}
    virtual void leftMouseDown(const renderer::Point& mouse) {}
    virtual void leftMouseDrag(const renderer::Point& mouse) {}

    renderer::Size getSize();
    void setWidth(int width);
    void setHeight(int width);
    renderer::Point getPosition();
    virtual void setPosition(const renderer::Point& position);
    virtual void layout() {}

protected:
    renderer::Size size{};
    renderer::Point position{};
};

}
