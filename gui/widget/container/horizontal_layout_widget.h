#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalLayoutWidget : public LayoutWidget {
public:
    HorizontalLayoutWidget(int padding_in_between);

    void layout() override;

    std::string_view className() const override {
        return "HorizontalLayoutWidget";
    }

private:
    int padding_in_between;
};

}  // namespace gui
