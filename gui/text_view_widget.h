#pragma once

#include "base/buffer.h"
#include "gui/widget.h"

namespace gui {

class TextViewWidget : public Widget {
public:
    TextViewWidget(std::shared_ptr<renderer::Renderer> renderer, const renderer::Size& size);

    void draw(const renderer::Size& screen_size, const renderer::Point& offset) override;
    void scroll(const renderer::Point& delta) override;

    void setContents(const std::string& text);

private:
    base::Buffer buffer;
    renderer::Point scroll_offset{};
    renderer::CaretInfo end_caret{};
};

}
