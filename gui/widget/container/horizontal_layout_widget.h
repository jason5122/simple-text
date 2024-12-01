#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalLayoutWidget : public LayoutWidget {
public:
    HorizontalLayoutWidget() = default;
    HorizontalLayoutWidget(const app::Size& size);

    void layout() override;

    std::string_view className() const override {
        return "HorizontalLayoutWidget";
    }
};

}  // namespace gui
