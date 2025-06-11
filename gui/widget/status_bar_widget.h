#pragma once

#include "container/horizontal_layout_widget.h"
#include "gui/widget/label_widget.h"

namespace gui {

class StatusBarWidget : public HorizontalLayoutWidget {
public:
    StatusBarWidget(int min_height, size_t font_id);

    void draw() override;

    void set_text(std::string_view str8);

    constexpr std::string_view class_name() const final override { return "StatusBarWidget"; }

private:
    static constexpr Point kLeftPadding{32, 0};
    // static constexpr Rgb kStatusBarColor{199, 203, 209};  // Light.
    static constexpr Rgb kStatusBarColor{46, 50, 56};  // Dark.
    // static constexpr Rgb kTextColor{64, 64, 64};     // Light.
    static constexpr Rgb kTextColor{217, 217, 217};  // Dark.

    // These cache unique_ptrs. These are guaranteed to be non-null since they are owned by
    // HorizontalLayoutWidget.
    LabelWidget* line_column_label;
};

}  // namespace gui
