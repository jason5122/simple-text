#pragma once

#include "base/buffer/piece_tree.h"
#include "gui/renderer/types.h"
#include "gui/text_system/selection.h"
#include "gui/types.h"
#include "gui/widget/scrollable_widget.h"

// TODO: Debug use; remove this.
// #define ENABLE_HIGHLIGHTING

#ifdef ENABLE_HIGHLIGHTING
#include "syntax_highlighter/highlighter.h"
#include "syntax_highlighter/language.h"
#include "syntax_highlighter/parse_tree.h"
#endif

namespace gui {

class TextEditWidget : public ScrollableWidget {
public:
    TextEditWidget(std::string_view str8, size_t font_id);

    void selectAll();
    void move(MoveBy by, bool forward, bool extend);
    void moveTo(MoveTo to, bool extend);
    void leftDelete();
    void rightDelete();
    void deleteWord(bool forward);
    std::string getSelectionText();
    void undo();
    void redo();
    void find(std::string_view str8);
    // TODO: Use a struct type for clarity.
    std::pair<size_t, size_t> getLineColumn();
    size_t getSelectionLength();

    // TODO: Refactor this.
    void updateFontId(size_t font_id);

    void draw() override;
    void leftMouseDown(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void leftMouseDrag(const Point& mouse_pos,
                       ModifierKey modifiers,
                       ClickType click_type) override;
    void leftMouseUp(const Point& mouse_pos) override;
    void insertText(std::string_view str8) override;

    void updateMaxScroll() override;

    constexpr CursorStyle cursorStyle() const final override {
        return CursorStyle::kIBeam;
    }
    constexpr std::string_view className() const final override {
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
    // TODO: Do we need this?
    static constexpr int kMinScrollbarWidth = 100;

    size_t font_id;

    base::PieceTree tree;

    Selection selection{};
    Selection old_selection{};

#ifdef ENABLE_HIGHLIGHTING
    highlight::ParseTree parse_tree;
#endif

    static constexpr int kGutterLeftPadding = 18 * 2;
    static constexpr int kGutterRightPadding = 8 * 2;

    size_t lineAtY(int y) const;
    inline const font::LineLayout& layoutAt(size_t line);
    inline constexpr Point textOffset();
    inline constexpr int gutterWidth();
    inline int lineNumberWidth();

    // Draw helpers.
    void renderText(int main_line_height, size_t start_line, size_t end_line);
    void renderSelections(int main_line_height, size_t start_line, size_t end_line);
    void renderScrollBars(int main_line_height);
    void renderCaret(int main_line_height);
};

}  // namespace gui
