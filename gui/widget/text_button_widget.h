#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class TextButtonWidget : public Widget {
public:
    TextButtonWidget(size_t font_id,
                     std::string_view str8,
                     Rgb bg_color,
                     const Size& padding,
                     const Size& min_size);

    void draw() override;

    constexpr std::string_view class_name() const final override { return "TextButtonWidget"; }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.

    Rgb bg_color;
    font::LineLayout line_layout;
    int line_height;

    constexpr Point text_center();
};

}  // namespace gui
