#pragma once

#include "gui/container_widget.h"

namespace gui {

class HorizontalLayoutWidget : public ContainerWidget {
public:
    HorizontalLayoutWidget(const renderer::Size& size);

    void draw() override;
    void layout() override;
};

}
