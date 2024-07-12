#pragma once

#include "base/buffer.h"
#include "gui/renderer/text/line_layout.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextViewWidget : public ScrollableWidget {
public:
    TextViewWidget() = default;
    TextViewWidget(const Size& size);

    void draw() override;
    void leftMouseDown(const Point& mouse_pos) override;
    void leftMouseDrag(const Point& mouse_pos) override;

    void updateMaxScroll() override;

    void setContents(const std::string& text);

private:
    base::Buffer buffer;
    CaretInfo start_caret{};
    CaretInfo end_caret{};

    std::vector<LineLayout::Token>::const_iterator start_caret_temp;
    std::vector<LineLayout::Token>::const_iterator end_caret_temp;

    // TODO: Clean this up.
    int longest_line_x = 0;
};

}
