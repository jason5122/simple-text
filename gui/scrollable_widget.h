#pragma once

#include "gui/widget.h"

namespace gui {

class ScrollableWidget : public Widget {
public:
    ScrollableWidget() {}
    ScrollableWidget(const renderer::Size& size) : Widget{size} {}

    virtual void updateMaxScroll() = 0;

    void scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) override;

protected:
    renderer::Point scroll_offset{};
    renderer::Point max_scroll_offset{};
};

}
