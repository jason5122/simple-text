#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextInputWidget : public ScrollableWidget {
public:
    TextInputWidget(size_t font_id, int top_padding, int left_padding);

    void draw() override;
    void updateMaxScroll() override;
    app::CursorStyle cursorStyle() const override;

    std::string_view className() const override {
        return "TextInputWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.
    // TODO: Add light variant.
    static constexpr Rgba kBackgroundColor{69, 75, 84, 255};  // Dark.

    size_t font_id;
    int top_padding;
    int left_padding;

    int line_height;
    std::string find_str = "needle";
};

}  // namespace gui
