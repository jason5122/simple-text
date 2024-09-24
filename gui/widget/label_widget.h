#pragma once

#include "font/types.h"
#include "gui/widget/widget.h"

namespace gui {

class LabelWidget : public Widget {
public:
    LabelWidget(const Size& size, int left_padding = 0, int right_padding = 0);

    void setText(std::string_view str8, const Rgb& color);
    void addLeftIcon(size_t icon_id);
    void addRightIcon(size_t icon_id);

    void draw(const Point& mouse_pos) override;

private:
    static constexpr Rgba kTempColor{223, 227, 230, 255};
    static constexpr Rgba kFolderIconColor{142, 142, 142, 255};

    int left_padding;
    int right_padding;

    font::LineLayout layout;
    Rgb color{};
    std::vector<size_t> left_side_icons;
    std::vector<size_t> right_side_icons;

    Point centerVertically(int widget_height);
};

}
