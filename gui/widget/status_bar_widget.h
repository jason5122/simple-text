#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(const Size& size);

    void draw(const Point& mouse_pos) override;
    void layout() override;

private:
    static constexpr Point kLeftPadding{32, 0};
    static constexpr Rgba kStatusBarColor{199, 203, 209, 255};

    std::unique_ptr<LabelWidget> line_column_label;
};

}
