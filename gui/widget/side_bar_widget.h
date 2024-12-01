#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/scrollable_widget.h"
#include <array>

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(const app::Size& size);

    void draw() override;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    bool mousePositionChanged(const std::optional<app::Point>& mouse_pos) override;
    void layout() override;

    void updateMaxScroll() override;

    std::string_view className() const override {
        return "SideBarWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{230, 230, 230};  // Dark.
    // static constexpr Rgba kSideBarColor{235, 237, 239, 255};  // Light.
    // static constexpr Rgba kScrollBarColor{190, 190, 190, 255};  // Light.
    static constexpr Rgba kSideBarColor{34, 38, 42, 255};       // Dark.
    static constexpr Rgba kScrollBarColor{105, 112, 118, 255};  // Dark.
    static constexpr Rgba kFolderIconColor{142, 142, 142, 255};
    static constexpr int kLeftPadding = 15 * 2;

    size_t label_font_id;
    static constexpr int kLabelFontSize = 22 * 2;

    static constexpr std::array strs = {"simple-text", ".cache", "app", "base", "build", "config"};
    std::unique_ptr<LabelWidget> folder_label;
    std::optional<size_t> hovered_index = std::nullopt;

    // Draw helpers.
    // TODO: Rename these methods.
    void renderOldLabel(int label_line_height);
    void renderNewLabel();
    void renderScrollBars(int line_height, size_t visible_lines);
};

}  // namespace gui
