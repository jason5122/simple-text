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

    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    size_t getCurrentIndex();
    void addTab(const std::string& text);
    void removeTab(size_t index);
    void selectAll();
    void move(MoveBy by, bool forward, bool extend);
    void moveTo(MoveTo to, bool extend);
    void insertText(std::string_view text);
    void leftDelete();
    std::string getSelectionText();

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
