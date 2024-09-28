#pragma once

#include "gui/text_system/line_layout_cache.h"
#include "gui/widget/label_widget.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(const Size& size);

    void draw(const Point& mouse_pos) override;
    void layout() override;

    void updateMaxScroll() override;

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

    // Draw helpers.
    // TODO: Rename these methods.
    void renderOldLabel(int ui_line_height);
    void renderNewLabel(const Point& mouse_pos);
    void renderScrollBars();
};

}
