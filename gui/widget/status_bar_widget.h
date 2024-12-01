#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class StatusBarWidget : public Widget {
public:
    StatusBarWidget(const app::Size& size, size_t font_id);

    void draw() override;
    void layout() override;

    std::string_view className() const override {
        return "StatusBarWidget";
    }

private:
    static constexpr app::Point kLeftPadding{32, 0};
    // static constexpr Rgba kStatusBarColor{199, 203, 209, 255};  // Light.
    static constexpr Rgba kStatusBarColor{46, 50, 56, 255};  // Dark.
    // static constexpr Rgb kTextColor{64, 64, 64};     // Light.
    static constexpr Rgb kTextColor{217, 217, 217};  // Dark.

    std::unique_ptr<LabelWidget> line_column_label;
};

}  // namespace gui
