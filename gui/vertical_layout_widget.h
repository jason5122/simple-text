#pragma once

#include "gui/container_widget.h"

namespace gui {

class VerticalLayoutWidget : public ContainerWidget {
public:
    VerticalLayoutWidget(const renderer::Size& size);

    void draw() override;
    void layout() override;
};

}
