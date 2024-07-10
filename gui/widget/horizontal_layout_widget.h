#pragma once

#include "gui/widget/container_widget.h"

namespace gui {

class HorizontalLayoutWidget : public ContainerWidget {
public:
    HorizontalLayoutWidget() = default;
    HorizontalLayoutWidget(const renderer::Size& size);

    void layout() override;
};

}
