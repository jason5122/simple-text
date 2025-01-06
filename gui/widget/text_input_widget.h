#pragma once

#include "gui/renderer/types.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextInputWidget : public ScrollableWidget {
public:
    TextInputWidget(size_t font_id, int top_padding, int left_padding);

    void draw() override;
    void updateMaxScroll() override;
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;

    constexpr app::CursorStyle cursorStyle() const final override {
        return app::CursorStyle::kIBeam;
    }
    constexpr std::string_view className() const final override {
        return "TextInputWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.
    // TODO: Add light variant.
    static constexpr Rgb kBackgroundColor{69, 75, 84};  // Dark.

    // static constexpr Rgb kCaretColor{95, 180, 180};       // Light.
    static constexpr Rgb kCaretColor{249, 174, 88};  // Dark.
    static constexpr int kCaretWidth = 4;

    size_t font_id;
    int top_padding;
    int left_padding;

    int line_height;
    std::string find_str = "needle";

    size_t caret = 0;

    inline const font::LineLayout& getLayout() const;
};

}  // namespace gui
