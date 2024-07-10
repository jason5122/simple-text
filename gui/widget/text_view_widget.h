#pragma once

#include "base/buffer.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class TextViewWidget : public ScrollableWidget {
public:
    TextViewWidget() = default;
    TextViewWidget(const renderer::Size& size);

    void draw() override;
    void leftMouseDown(const renderer::Point& mouse_pos) override;
    void leftMouseDrag(const renderer::Point& mouse_pos) override;

    void updateMaxScroll() override;

    void setContents(const std::string& text);

private:
    base::Buffer buffer;
    renderer::CaretInfo start_caret{};
    renderer::CaretInfo end_caret{};
};

}
