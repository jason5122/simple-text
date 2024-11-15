#pragma once

#include "base/buffer/piece_tree.h"
#include "gui/renderer/opengl_types.h"
#include "gui/text_system/line_layout_cache.h"
#include "gui/text_system/selection.h"
#include "gui/widget/scrollable_widget.h"
#include "gui/widget/types.h"

// TODO: Debug use; remove this.
#define ENABLE_HIGHLIGHTING

#ifdef ENABLE_HIGHLIGHTING
#include "syntax_highlighter/language.h"
#include "syntax_highlighter/parse_tree.h"
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

    void draw() override;
    void leftMouseDown(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const app::Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;

    void updateMaxScroll() override;

    app::CursorStyle cursorStyle() const override {
        return app::CursorStyle::kIBeam;
    }
    std::string_view className() const override {
        return "TextViewWidget";
    };

private:
    static constexpr int kMinScrollbarWidth = 100;
    // static constexpr Rgb kTextColor{51, 51, 51};     // Light.
    static constexpr Rgb kTextColor{216, 222, 233};  // Dark.
    // static constexpr Rgb kLineNumberColor{152, 152, 152};  // Light.
    static constexpr Rgb kLineNumberColor{130, 139, 150};  // Dark.
    // static constexpr Rgb kSelectedLineNumberColor{81, 81, 81};     // Light.
    static constexpr Rgb kSelectedLineNumberColor{190, 197, 209};  // Dark.
    // static constexpr Rgba kGutterColor{227, 230, 232, 255};  // Light.
    // static constexpr Rgba kScrollBarColor{190, 190, 190, 255};  // Light.
    static constexpr Rgba kGutterColor{77, 88, 100, 255};       // Dark.
    static constexpr Rgba kScrollBarColor{105, 112, 118, 255};  // Dark.
    // static constexpr Rgba kCaretColor{95, 180, 180, 255};       // Light.
    static constexpr Rgba kCaretColor{249, 174, 88, 255};  // Dark.
    static constexpr int kCaretWidth = 4;

    base::PieceTree tree;
    LineLayoutCache line_layout_cache;

    Selection selection{};

#ifdef ENABLE_HIGHLIGHTING
    highlight::ParseTree parse_tree;
    highlight::Language language;
#endif

    static constexpr int kGutterLeftPadding = 18 * 2;
    static constexpr int kGutterRightPadding = 8 * 2;
    // TODO: Why is this more accurate?
    // static constexpr int kGutterLeftPadding = 37;
    // static constexpr int kGutterRightPadding = 15;

    size_t lineAtY(int y) const;
    inline const font::LineLayout& layoutAt(size_t line);
    inline constexpr app::Point textOffset();
    inline constexpr int gutterWidth();
    inline constexpr int lineNumberWidth();

    // Draw helpers.
    void renderText(size_t start_line, size_t end_line, int main_line_height);
    void renderSelections(size_t start_line, size_t end_line);
    void renderScrollBars(int main_line_height);
    void renderCaret(int main_line_height);
};

}  // namespace gui
