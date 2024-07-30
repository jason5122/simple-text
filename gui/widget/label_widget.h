#pragma once

#include "font/font_rasterizer.h"
#include "gui/widget/widget.h"

namespace gui {

class LabelWidget : public Widget {
public:
    LabelWidget(const Size& size, int left_padding = 0, int right_padding = 0);

    void setText(const std::string& str8, const Rgb& color);
    void addLeftIcon(size_t icon_id);
    void addRightIcon(size_t icon_id);

    void draw() override;

private:
    int left_padding;
    int right_padding;

    font::FontRasterizer::LineLayout label_line_layout;
    Rgb color{};
    std::vector<size_t> left_side_icons;
    std::vector<size_t> right_side_icons;

    Point centerVertically(int widget_height);
};

}
