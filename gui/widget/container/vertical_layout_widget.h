#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class VerticalLayoutWidget : public LayoutWidget {
public:
    VerticalLayoutWidget() = default;
    VerticalLayoutWidget(const app::Size& size);

    void layout() override;

    std::string_view getClassName() const override {
        return "VerticalLayoutWidget";
    };
};

}  // namespace gui
