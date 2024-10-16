#pragma once

#include "gui/text_system/line_layout_cache.h"
#include "gui/widget/label_widget.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(const Size& size);

    void draw(const std::optional<Point>& mouse_pos) override;
    void leftMouseDrag(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    bool mousePositionChanged(const std::optional<Point>& mouse_pos) override;
    void layout() override;

    void updateMaxScroll() override;

    std::string_view getClassName() const override {
        return "SideBarWidget";
    };

private:
    static constexpr Rgb kTextColor{51, 51, 51};
    static constexpr Rgba kSideBarColor{235, 237, 239, 255};
    static constexpr Rgba kScrollBarColor{190, 190, 190, 255};
    static constexpr Rgba kFolderIconColor{142, 142, 142, 255};
    static constexpr int kLeftPadding = 15 * 2;

    size_t label_font_id;
    static constexpr std::array strs = {"simple-text", ".cache", "app", "base", "build", "config"};
    LineLayoutCache line_layout_cache;
    std::unique_ptr<LabelWidget> folder_label;
    std::optional<size_t> hovered_index = std::nullopt;

    // Draw helpers.
    // TODO: Rename these methods.
    void renderOldLabel(int label_line_height);
    void renderNewLabel(const std::optional<Point>& mouse_pos);
    void renderScrollBars(int line_height, size_t visible_lines);
};

}
