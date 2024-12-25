#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(size_t font_id, int height, size_t panel_close_image_id);

    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    void lastIndex();
    void addTab(std::string_view title);
    void removeTab(size_t index);

    void draw() override;
    void layout() override;

    std::string_view className() const override {
        return "TabBarWidget";
    }

private:
    static constexpr int kTabWidth = 360;
    static constexpr int kTabCornerRadius = 10;
    static constexpr app::Size kTabSeparatorSize{.width = 2, .height = 38};
    // static constexpr Rgba kTabBarColor{190, 190, 190, 255};  // Light.
    // static constexpr Rgba kTabColor{253, 253, 253, 255};  // Light.
    static constexpr Rgba kTabBarColor{79, 86, 94, 255};  // Dark.
    static constexpr Rgba kTabColor{48, 56, 65, 255};     // Dark.
    static constexpr Rgba kTabSeparatorColor{148, 149, 149, 255};
    // static constexpr Rgb kTabTextColor{92, 92, 92};  // Light.
    static constexpr Rgb kTabTextColor{255, 255, 255};  // Dark.

    size_t font_id;
    size_t panel_close_image_id;
    size_t index = 0;
    std::vector<std::unique_ptr<LabelWidget>> tab_name_labels;
};

}  // namespace gui
