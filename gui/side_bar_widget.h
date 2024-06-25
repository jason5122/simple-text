#pragma once

#include "gui/widget.h"

namespace gui {

class SideBarWidget : public Widget {
public:
    SideBarWidget(const renderer::Size& size);

    void draw() override;
};

}
