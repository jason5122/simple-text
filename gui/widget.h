#pragma once

#include "renderer/renderer.h"
#include <memory>

namespace gui {

class Widget {
public:
    Widget(std::shared_ptr<renderer::Renderer> renderer, const renderer::Size& size)
        : renderer{std::move(renderer)}, size{size} {}
    virtual ~Widget() {}

    virtual void draw(const renderer::Size& screen_size, const renderer::Point& offset) = 0;
    virtual void scroll(const renderer::Point& delta) = 0;

protected:
    std::shared_ptr<renderer::Renderer> renderer;
    renderer::Size size;
};

}
