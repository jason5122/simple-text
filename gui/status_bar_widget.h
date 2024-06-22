#pragma once

#include "gui/widget.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(const renderer::Size& size);

    void draw(const renderer::Size& screen_size, const renderer::Point& offset) override;
};

}
