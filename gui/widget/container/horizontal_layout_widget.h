#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalLayoutWidget : public LayoutWidget {
public:
    HorizontalLayoutWidget(int padding_in_between, int left_padding = 0, int right_padding = 0);

    void layout() override;

    std::string_view className() const override {
        return "HorizontalLayoutWidget";
    }

private:
    int padding_in_between;
    int left_padding;
    int right_padding;
};

}  // namespace gui
