#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/scrollable_widget.h"
#include <array>

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(int width);

    void draw() override;
    bool mousePositionChanged(const std::optional<app::Point>& mouse_pos) override;

    void updateMaxScroll() override;

    constexpr std::string_view className() const final override {
        return "SideBarWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{230, 230, 230};  // Dark.
    // static constexpr Rgba kSideBarColor{235, 237, 239, 255};  // Light.
    static constexpr Rgba kSideBarColor{34, 38, 42, 255};  // Dark.
    // static constexpr Rgba kScrollBarColor{190, 190, 190, 255};  // Light.
    static constexpr Rgba kScrollBarColor{105, 112, 118, 255};  // Dark.
    static constexpr Rgba kFolderIconColor{142, 142, 142, 255};
    static constexpr Rgba kHighlightColor = {255, 255, 0, 255};

    static constexpr int kLeftPadding = 15 * 2;

    size_t label_font_id;
    static constexpr int kLabelFontSize = 22 * 2;

    static constexpr std::array strs = {"simple-text", ".cache", "app", "base", "build", "config"};
    std::optional<size_t> hovered_index = std::nullopt;

    // Draw helpers.
    // TODO: Rename these methods.
    void renderLabel();
    void renderScrollBars(int line_height, size_t visible_lines);
};

}  // namespace gui
