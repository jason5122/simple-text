#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(const app::Size& size);

    void draw(const std::optional<app::Point>& mouse_pos) override;
    void layout() override;

    std::string_view getClassName() const override {
        return "StatusBarWidget";
    };

private:
    static constexpr app::Point kLeftPadding{32, 0};
    // static constexpr Rgba kStatusBarColor{199, 203, 209, 255};  // Light.
    static constexpr Rgba kStatusBarColor{46, 50, 56, 255};  // Dark.

    std::unique_ptr<LabelWidget> line_column_label;
};

}  // namespace gui
