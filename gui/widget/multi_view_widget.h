#pragma once

#include "gui/widget/text_view_widget.h"
#include "gui/widget/types.h"
#include "gui/widget/widget.h"
#include <memory>
#include <vector>

namespace gui {

class MultiViewWidget : public Widget {
public:
    MultiViewWidget();

    void setIndex(int index);
    void prevIndex();
    void nextIndex();
    void addTab(const std::string& text);
    void selectAll();
    void move(MovementKind kind, bool forward);

    void draw() override;
    void scroll(const Point& mouse_pos, const Point& delta) override;
    void leftMouseDown(const Point& mouse_pos) override;
    void leftMouseDrag(const Point& mouse_pos) override;
    void layout() override;

private:
    int index = 0;
    std::vector<std::shared_ptr<TextViewWidget>> text_views;
};

}
