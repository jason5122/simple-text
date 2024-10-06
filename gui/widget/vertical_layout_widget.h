#pragma once

#include "gui/widget/container_widget.h"

namespace gui {

class VerticalLayoutWidget : public ContainerWidget {
public:
    VerticalLayoutWidget() = default;
    VerticalLayoutWidget(const Size& size);

    void layout() override;

    std::string_view getClassName() const override {
        return "VerticalLayoutWidget";
    };
};

}
