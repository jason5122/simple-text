#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextInputWidget : public ScrollableWidget {
public:
    TextInputWidget(size_t font_id);

    void draw() override;
    void updateMaxScroll() override;

    std::string_view className() const override {
        return "TextInputWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.

    size_t font_id;
    int line_height;
    std::string find_str = "needle";
};

}  // namespace gui
