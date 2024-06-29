#pragma once

#include "gui/container_widget.h"

namespace gui {

class VerticalLayoutWidget : public ContainerWidget {
public:
    VerticalLayoutWidget() = default;
    VerticalLayoutWidget(const renderer::Size& size);

    void layout() override;
};

}
