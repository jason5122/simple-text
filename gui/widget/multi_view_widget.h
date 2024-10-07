#pragma once

#include "gui/widget/container_widget.h"
#include "gui/widget/text_view_widget.h"
#include <memory>
#include <vector>

namespace gui {

class MultiViewWidget : public ContainerWidget {
public:
    MultiViewWidget();

    TextViewWidget* currentTextViewWidget() const;
    void setIndex(size_t index);
    void prevIndex();
    void nextIndex();
    void lastIndex();
    size_t getCurrentIndex();
    void addTab(std::string_view text);
    void removeTab(size_t index);

    void draw(const std::optional<Point>& mouse_pos) override;
    void scroll(const Point& mouse_pos, const Point& delta) override;
    void leftMouseDown(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void leftMouseDrag(const Point& mouse_pos,
                       app::ModifierKey modifiers,
                       app::ClickType click_type) override;
    void layout() override;
    Widget* getWidgetAtPosition(const Point& pos) override;

    std::string_view getClassName() const override {
        return "MultiViewWidget";
    };

private:
    size_t index = 0;
    std::vector<std::shared_ptr<TextViewWidget>> text_views;
};

}
