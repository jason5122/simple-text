#pragma once

#include "base/buffer/piece_tree.h"
#include "gui/renderer/types.h"
#include "gui/text_system/selection.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextInputWidget : public ScrollableWidget {
public:
    TextInputWidget(size_t font_id, int top_padding, int left_padding);

    void draw() override;
    void updateMaxScroll() override;
    void leftMouseDown(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void insertText(std::string_view str8) override;

    constexpr bool canBeFocused() const override {
        return true;
    }
    constexpr CursorStyle cursorStyle() const final override {
        return CursorStyle::kIBeam;
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
    static constexpr int kBorderThickness = 2;

    size_t font_id;
    int top_padding;
    int left_padding;

    int line_height;
    base::PieceTree tree;
    Selection selection{};

    size_t lineAtY(int y) const;
    inline const font::LineLayout& layoutAt(size_t line);
    inline constexpr Point textOffset();
};

}  // namespace gui
