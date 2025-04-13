#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class TabBarLabelWidget : public Widget {
public:
    TabBarLabelWidget(size_t font_id,
                      const Size& size,
                      int left_padding = 0,
                      int right_padding = 0);

    void set_text(std::string_view str8);
    void set_color(const Rgb& color);
    void add_left_icon(size_t icon_id);
    void add_right_icon(size_t icon_id);

    void draw() override;

    constexpr std::string_view class_name() const final override { return "TabBarLabelWidget"; }

private:
    static constexpr Rgb kTempColor{223, 227, 230};
    static constexpr Rgb kFolderIconColor{142, 142, 142};

    size_t font_id;
    int left_padding;
    int right_padding;

    std::string label_str;
    Rgb color{};
    std::vector<size_t> left_side_icons;
    std::vector<size_t> right_side_icons;

    Point center_vertically(int widget_height);
};

}  // namespace gui
