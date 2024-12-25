#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/widget.h"

namespace gui {

class TextButtonWidget : public Widget {
public:
    // TODO: Accept a "minimum size" argument.
    TextButtonWidget(size_t font_id, std::string_view str8, Rgba bg_color);

    void draw() override;

    std::string_view className() const override {
        return "TextButtonWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.

    Rgba bg_color;
    font::LineLayout line_layout;
};

}  // namespace gui
