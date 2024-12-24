#pragma once

#include "base/buffer/piece_tree.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/scrollable_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class FindPanelWidget : public ScrollableWidget {
public:
    FindPanelWidget(const app::Size& size, size_t font_id);

    void draw() override;
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
    static constexpr int kTextInputPadding = 8;

    size_t font_id;
    base::PieceTree tree;

    inline const font::LineLayout& layoutAt(size_t line);
};

}  // namespace gui
