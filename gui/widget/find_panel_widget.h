#pragma once

#include "base/buffer/piece_tree.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/container/horizontal_layout_widget.h"
#include "gui/widget/scrollable_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class FindPanelWidget : public ScrollableWidget {
public:
    FindPanelWidget(const app::Size& size,
                    size_t main_font_id,
                    size_t ui_font_id,
                    size_t icon_regex_image_id,
                    size_t icon_case_sensitive_image_id,
                    size_t icon_whole_word_image_id,
                    size_t icon_wrap_image_id,
                    size_t icon_in_selection_id,
                    size_t icon_highlight_matches_id);

    void draw() override;
    void layout() override;
    void updateMaxScroll() override;

    std::string_view className() const override {
        return "FindWidget";
    }

private:
    // static constexpr Rgba kFindPanelColor{199, 203, 209, 255};  // Light.
    static constexpr Rgba kFindPanelColor{46, 50, 56, 255};  // Dark.
    // static constexpr Rgba kFindPanelColor{255, 0, 0, 255};  // TODO: Remove this.
    // static constexpr Rgba kTextInputColor{255, 255, 255, 255};  // Light.
    static constexpr Rgba kTextInputColor{69, 75, 84, 255};  // Dark.
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.
    // static constexpr Rgba kIconBackgroundColor{216, 222, 233, 255};  // Light.
    static constexpr Rgba kIconBackgroundColor{69, 75, 84, 255};  // Dark.

    static constexpr Rgba kIconColor{236, 237, 238, false};  // Dark.
    static constexpr int kTextInputPadding = 8;

    size_t main_font_id;
    base::PieceTree tree;

    std::vector<size_t> image_ids;
    int image_offset_x = 0;

    std::shared_ptr<HorizontalLayoutWidget> horizontal_layout;

    inline const font::LineLayout& layoutAt(size_t line);
    constexpr app::Point textInputOffset();

    // Draw helpers.
    void renderText(int line_height);
    void renderSelections(int line_height);
    void renderImages();
};

}  // namespace gui
