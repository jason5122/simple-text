#pragma once

#include "container/horizontal_layout_widget.h"
#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class StatusBarWidget : public HorizontalLayoutWidget {
public:
    StatusBarWidget(int min_height, size_t font_id);

    void draw() override;

    void setText(std::string_view str8);

    constexpr std::string_view className() const final override {
        return "StatusBarWidget";
    }

private:
    static constexpr app::Point kLeftPadding{32, 0};
    // static constexpr Rgba kStatusBarColor{199, 203, 209, 255};  // Light.
    static constexpr Rgba kStatusBarColor{46, 50, 56, 255};  // Dark.
    // static constexpr Rgb kTextColor{64, 64, 64};     // Light.
    static constexpr Rgb kTextColor{217, 217, 217};  // Dark.

    std::shared_ptr<LabelWidget> line_column_label;
};

}  // namespace gui
