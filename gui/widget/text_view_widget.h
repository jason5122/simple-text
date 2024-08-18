#pragma once

#include "base/buffer/piece_table.h"
#include "gui/text_system/caret.h"
#include "gui/text_system/line_layout_cache.h"
#include "gui/widget/scrollable_widget.h"
#include "gui/widget/types.h"

namespace gui {

class TextViewWidget : public ScrollableWidget {
public:
    TextViewWidget(const std::string& text);

    void selectAll();
    void move(MoveBy by, bool forward, bool extend);
    void moveTo(MoveTo to, bool extend);
    void insertText(std::string_view text);
    void leftDelete();

    void draw() override;
    void leftMouseDown(const Point& mouse_pos) override;
    void leftMouseDrag(const Point& mouse_pos) override;

    void updateMaxScroll() override;

private:
    static constexpr int kMinScrollbarWidth = 100;
    static constexpr Rgb kTextColor{51, 51, 51};
    static constexpr Rgba kScrollBarColor{190, 190, 190, 255};
    static constexpr Rgba kCaretColor{95, 180, 180, 255};

    base::PieceTable table;
    LineLayoutCache line_layout_cache;

    Caret start_caret{};
    Caret end_caret{};

    size_t lineAtY(int y);
    inline const font::LineLayout& layoutAt(size_t line);
    inline const font::LineLayout& layoutAt(size_t line, bool& exclude_end);
};

}
