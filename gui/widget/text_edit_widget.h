#pragma once

#include "base/buffer/piece_tree.h"
#include "editor/selection.h"
#include "gui/renderer/types.h"
#include "gui/types.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextEditWidget : public ScrollableWidget {
public:
    TextEditWidget(std::string_view str8, size_t font_id);

    // Editing methods.
    void select_all();
    void move(MoveBy by, bool forward, bool extend);
    void move_to(MoveTo to, bool extend);
    void left_delete();
    void right_delete();
    void delete_word(bool forward);
    std::string get_selection_text();
    void undo();
    void redo();
    void find(std::string_view str8);
    // TODO: Use a struct type for clarity.
    std::pair<size_t, size_t> get_line_column();
    size_t get_selection_length();

    // TODO: Refactor this.
    void update_font_id(size_t font_id);

    void draw() override;
    void left_mouse_down(const Point& mouse_pos,
                         ModifierKey modifiers,
                         ClickType click_type) override;
    void left_mouse_drag(const Point& mouse_pos,
                         ModifierKey modifiers,
                         ClickType click_type) override;
    void left_mouse_up(const Point& mouse_pos) override;
    void insert_text(std::string_view str8) override;

    void update_max_scroll() override;

    constexpr CursorStyle cursor_style() const final override {
        return CursorStyle::kIBeam;
    }
    constexpr std::string_view class_name() const final override {
        return "TextViewWidget";
    }

private:
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.
    // static constexpr Rgb kLineNumberColor{152, 152, 152};  // Light.
    static constexpr Rgb kLineNumberColor{130, 139, 150};  // Dark.
    // static constexpr Rgb kSelectedLineNumberColor{81, 81, 81};     // Light.
    static constexpr Rgb kSelectedLineNumberColor{190, 197, 209};  // Dark.
    // static constexpr Rgb kGutterColor{227, 230, 232};  // Light.
    // static constexpr Rgb kScrollBarColor{190, 190, 190};  // Light.
    static constexpr Rgb kGutterColor{77, 88, 100};       // Dark.
    static constexpr Rgb kScrollBarColor{105, 112, 118};  // Dark.
    // static constexpr Rgb kCaretColor{95, 180, 180};       // Light.
    static constexpr Rgb kCaretColor{249, 174, 88};  // Dark.
    // TODO: Implement light version.
    static constexpr Rgb kShadowColor{42, 49, 57};  // Dark.
    static constexpr int kCaretWidth = 4;
    static constexpr int kExtraPadding = 8;
    static constexpr int kBorderThickness = 2;
    // TODO: Change minimum values.
    static constexpr int kMinScrollBarWidth = 100;
    static constexpr int kMinScrollBarHeight = 30;
    static constexpr int kScrollBarThickness = 7 * 2;
    static constexpr int kScrollBarPadding = 8;

    size_t font_id;

    base::PieceTree tree;

    Selection selection;
    Selection old_selection;

    static constexpr int kGutterLeftPadding = 18 * 2;
    static constexpr int kGutterRightPadding = 8 * 2;

    size_t line_at_y(int y) const;
    inline const font::LineLayout& layout_at(size_t line);
    inline constexpr Point text_offset();
    inline constexpr int gutter_width();
    inline int line_number_width();

    // Draw helpers.
    void render_text(int main_line_height, size_t start_line, size_t end_line);
    void render_selections(int main_line_height, size_t start_line, size_t end_line);
    void render_scroll_bars(int main_line_height);
    void render_caret(int main_line_height);
};

}  // namespace gui
