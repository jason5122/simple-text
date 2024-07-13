#pragma once

#include "gui/widget/text_view_widget.h"
#include "gui/widget/widget.h"
#include <memory>
#include <vector>

namespace gui {

class MultiViewWidget : public Widget {
public:
    MultiViewWidget();

    void addTextView(std::shared_ptr<TextViewWidget> text_view);
    void setIndex(size_t index);
    void setContents(const std::string& text);

    void draw() override;
    void scroll(const Point& mouse_pos, const Point& delta) override;
    void leftMouseDown(const Point& mouse_pos) override;
    void leftMouseDrag(const Point& mouse_pos) override;
    void layout() override;

private:
    size_t index = 0;
    std::vector<std::shared_ptr<TextViewWidget>> text_views;
};

}
