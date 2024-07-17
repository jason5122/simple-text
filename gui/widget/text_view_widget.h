#pragma once

#include "base/buffer/buffer.h"
#include "gui/renderer/text/line_layout.h"
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
    base::Buffer buffer;
    LineLayout line_layout;

    LineLayout::Iterator start_caret;
    LineLayout::Iterator end_caret;

    // We use this value to position the caret during vertical movement.
    // This is updated whenever the caret moves horizontally.
    int caret_x = 0;
    void updateCaretX();
    size_t lineAtPoint(const Point& point);
};

}
