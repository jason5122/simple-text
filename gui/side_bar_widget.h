#pragma once

#include "gui/widget.h"

namespace gui {

class SideBarWidget : public Widget {
public:
    SideBarWidget(std::shared_ptr<renderer::Renderer> renderer, int side_bar_width);

    void draw(int width, int height) override;

private:
    int side_bar_width;
};

}
