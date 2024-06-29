#pragma once

#include "base/buffer.h"
#include "gui/widget.h"

namespace gui {

class TextViewWidget : public Widget {
public:
    TextViewWidget() = default;
    TextViewWidget(const renderer::Size& size);

    void draw() override;
    void scroll(const renderer::Point& delta) override;
    void leftMouseDown(const renderer::Point& mouse_pos) override;
    void leftMouseDrag(const renderer::Point& mouse_pos) override;

    void setContents(const std::string& text);

private:
    base::Buffer buffer;
    renderer::Point scroll_offset{};
    renderer::CaretInfo start_caret{};
    renderer::CaretInfo end_caret{};
};

}
