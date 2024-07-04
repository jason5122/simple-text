#pragma once

#include "gui/widget.h"

namespace gui {

class SideBarWidget : public Widget {
public:
    SideBarWidget(const renderer::Size& size);

    void draw() override;
    void scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) override;

private:
    renderer::Point scroll_offset{};
};

}
