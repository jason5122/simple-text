#pragma once

#include "base/buffer/piece_table.h"
#include "gui/renderer/text/caret.h"
#include "gui/renderer/text/line_layout_cache.h"
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
    base::PieceTable table;
    LineLayoutCache line_layout_cache;

    Caret start_caret{};
    Caret end_caret{};

    // We use this value to position the caret during vertical movement.
    // This is updated whenever the caret moves horizontally.
    int caret_x = 0;
    void updateCaretX();
    size_t lineAtPoint(const Point& point);
};

}
