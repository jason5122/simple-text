#pragma once

#include "gui/widget.h"

namespace gui {

class SideBarWidget : public Widget {
public:
    SideBarWidget(std::shared_ptr<renderer::Renderer> renderer, int side_bar_width);

    void draw(const renderer::Size& size, const renderer::Point& offset) override;
    void scroll(const renderer::Point& delta) override;

private:
    int side_bar_width;
};

}
