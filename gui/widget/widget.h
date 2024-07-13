#pragma once

#include "gui/renderer/types.h"

namespace gui {

class Widget {
public:
    Widget() {}
    Widget(const Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw() = 0;
    virtual void scroll(const Point& mouse_pos, const Point& delta) {}
    virtual void leftMouseDown(const Point& mouse_pos) {}
    virtual void leftMouseDrag(const Point& mouse_pos) {}
    virtual void layout() {}
    virtual void setPosition(const Point& pos);
    virtual Widget* getWidgetAtPosition(const Point& pos);

    Size getSize();
    void setSize(const Size& size);
    void setWidth(int width);
    void setHeight(int width);
    Point getPosition();
    bool hitTest(const Point& point);

protected:
    Size size{};
    Point position{};
};

}
