#pragma once

#include "base/buffer/piece_tree.h"
#include "gui/text_system/line_layout_cache.h"
#include "gui/text_system/selection.h"
#include "gui/widget/scrollable_widget.h"
#include "gui/widget/types.h"

// TODO: Debug use; remove this.
// #define ENABLE_HIGHLIGHTING

#ifdef ENABLE_HIGHLIGHTING
#include "base/syntax_highlighter/syntax_highlighter.h"
#endif

namespace gui {

class TextViewWidget : public ScrollableWidget {
public:
    TextViewWidget(std::string_view text);

    void selectAll();
    void move(MoveBy by, bool forward, bool extend);
    void moveTo(MoveTo to, bool extend);
    void insertText(std::string_view text);
    void leftDelete();
    void rightDelete();
    void deleteWord(bool forward);
    std::string getSelectionText();
    void undo();
    void redo();

    void draw(const std::optional<Point>& mouse_pos) override;
    void leftMouseDown(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;

    void updateMaxScroll() override;

    app::CursorStyle getCursorStyle() const override {
        return app::CursorStyle::kIBeam;
    }
    std::string_view getClassName() const override {
        return "TextViewWidget";
    };

private:
    static constexpr int kMinScrollbarWidth = 100;
    static constexpr Rgb kTextColor{51, 51, 51};
    static constexpr Rgb kLineNumberColor{152, 152, 152};
    static constexpr Rgb kSelectedLineNumberColor{81, 81, 81};
    // static constexpr Rgba kGutterColor{227, 230, 232, 255};  // Light.
    // static constexpr Rgba kScrollBarColor{190, 190, 190, 255};  // Light.
    static constexpr Rgba kGutterColor{77, 88, 100, 255};       // Dark.
    static constexpr Rgba kScrollBarColor{105, 112, 118, 255};  // Dark.
    static constexpr Rgba kCaretColor{95, 180, 180, 255};

    base::PieceTree tree;
    LineLayoutCache line_layout_cache;

    Selection selection{};

#ifdef ENABLE_HIGHLIGHTING
    base::SyntaxHighlighter highlighter;
#endif

    static constexpr int kGutterLeftPadding = 23 * 2;
    int line_number_width;
    static constexpr int kGutterRightPadding = 8 * 2;

    size_t lineAtY(int y);
    inline const font::LineLayout& layoutAt(size_t line);
    inline const font::LineLayout& layoutAt(size_t line, bool& exclude_end);
    inline constexpr Point textOffset() const;
    inline constexpr int gutterWidth() const;

    // Draw helpers.
    void renderText(size_t start_line, size_t end_line, int main_line_height);
    void renderSelections(size_t start_line, size_t end_line);
    void renderScrollBars(int main_line_height);
    void renderCaret(int main_line_height);
};

}  // namespace gui
