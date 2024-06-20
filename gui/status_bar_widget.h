#pragma once

#include "gui/widget.h"
#include "renderer/renderer.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(std::shared_ptr<renderer::Renderer> renderer, const renderer::Size& size);

    void draw(const renderer::Size& screen_size, const renderer::Point& offset) override;
    void scroll(const renderer::Point& delta) override;

private:
    std::shared_ptr<renderer::Renderer> renderer;
};

}
