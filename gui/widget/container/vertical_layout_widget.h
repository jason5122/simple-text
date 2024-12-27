#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class VerticalLayoutWidget : public LayoutWidget {
public:
    VerticalLayoutWidget() = default;

    void layout() override;

    constexpr std::string_view className() const override {
        return "VerticalLayoutWidget";
    }
};

}  // namespace gui
