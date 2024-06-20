#pragma once

#include "renderer/types.h"

namespace gui {

class Widget {
public:
    Widget(const renderer::Size& size) : size{size} {}
    virtual ~Widget() {}

    virtual void draw(const renderer::Size& screen_size, const renderer::Point& offset) = 0;
    virtual void scroll(const renderer::Point& delta) = 0;

    renderer::Size getSize() {
        return size;
    }

protected:
    renderer::Size size;
};

}
