#pragma once

#include "gui/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(const renderer::Size& size);

    void draw() override;
};

}
