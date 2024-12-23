#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalPartitionWidget : public LayoutWidget {
public:
    HorizontalPartitionWidget() = default;
    HorizontalPartitionWidget(const app::Size& size);

    void layout() override;

    std::string_view className() const override {
        return "HorizontalPartitionWidget";
    }
};

}  // namespace gui
