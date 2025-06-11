#pragma once

#include "gui/widget/container/horizontal_layout_widget.h"
#include "gui/widget/text_input_widget.h"

namespace gui {

class FindPanelWidget : public HorizontalLayoutWidget {
public:
    FindPanelWidget(size_t main_font_id,
                    size_t ui_font_id,
                    size_t icon_regex_image_id,
                    size_t icon_case_sensitive_image_id,
                    size_t icon_whole_word_image_id,
                    size_t icon_wrap_image_id,
                    size_t icon_in_selection_id,
                    size_t icon_highlight_matches_id,
                    size_t panel_close_image_id);

    void draw() override;

    constexpr std::string_view class_name() const final override { return "FindPanelWidget"; }

private:
    // static constexpr Rgb kFindPanelColor{199, 203, 209};  // Light.
    static constexpr Rgb kFindPanelColor{46, 50, 56};  // Dark.
    // static constexpr Rgb kIconBackgroundFocusedColor{216, 222, 233};  // Light.
    static constexpr Rgb kIconBackgroundFocusedColor{69, 75, 84};  // Dark.
    // TODO: Add light variant.
    static constexpr Rgb kIconBackgroundColor{60, 65, 73};  // Dark.
    // TODO: Add light variant.
    static constexpr Rgb kIconColor{236, 237, 238};  // Dark.
    // TODO: Add light variant.
    static constexpr Rgb kCloseIconColor{130, 132, 136};  // Dark.

    static constexpr int kHorizontalPadding = 4;
    static constexpr int kVerticalPadding = 12;

    // These cache unique_ptrs. These are guaranteed to be non-null since they are owned by
    // HorizontalLayoutWidget.
    TextInputWidget* text_input_widget;
};

}  // namespace gui
