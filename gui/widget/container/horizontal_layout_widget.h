#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalLayoutWidget : public LayoutWidget {
public:
    HorizontalLayoutWidget(int padding_in_between = 0,
                           int left_padding = 0,
                           int right_padding = 0,
                           int top_padding = 0,
                           int bottom_padding = 0);

    void layout() override;

    constexpr std::string_view class_name() const override { return "HorizontalLayoutWidget"; }

private:
    int padding_in_between;
    int left_padding;
    int right_padding;
    int top_padding;
    int bottom_padding;
};

}  // namespace gui
