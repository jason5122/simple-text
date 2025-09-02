#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/tab_bar_label_widget.h"
#include "gui/widget/widget.h"

namespace gui {

class TabBarWidget : public Widget {
public:
    TabBarWidget(size_t font_id, int height, size_t panel_close_image_id);

    void set_index(size_t index);
    void prev_index();
    void next_index();
    void last_index();
    void add_tab(std::string_view title);
    void remove_tab(size_t index);

    void draw() override;
    void layout() override;

    constexpr std::string_view class_name() const final override { return "TabBarWidget"; }

private:
    static constexpr int kTabWidth = 360;
    static constexpr int kTabCornerRadius = 10;
    static constexpr Size kTabSeparatorSize = {.width = 2, .height = 38};
    // static constexpr Rgb kTabBarColor = {190, 190, 190};  // Light.
    // static constexpr Rgb kTabColor = {253, 253, 253};  // Light.
    static constexpr Rgb kTabBarColor = {79, 86, 94};  // Dark.
    static constexpr Rgb kTabColor = {48, 56, 65};     // Dark.
    static constexpr Rgb kTabSeparatorColor = {148, 149, 149};
    // static constexpr Rgb kTabTextColor = {92, 92, 92};  // Light.
    static constexpr Rgb kTabTextColor = {255, 255, 255};  // Dark.

    size_t font_id_;
    size_t panel_close_image_id_;
    size_t index_ = 0;
    std::vector<std::unique_ptr<TabBarLabelWidget>> labels_;
};

}  // namespace gui
