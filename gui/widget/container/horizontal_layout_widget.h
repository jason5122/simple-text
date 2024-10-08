#pragma once

#include "gui/widget/container/layout_widget.h"

namespace gui {

class HorizontalLayoutWidget : public LayoutWidget {
public:
    HorizontalLayoutWidget() = default;
    HorizontalLayoutWidget(const Size& size);

    void layout() override;

    std::string_view getClassName() const override {
        return "HorizontalLayoutWidget";
    };
};

}
