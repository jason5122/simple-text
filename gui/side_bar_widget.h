#pragma once

#include "gui/label_widget.h"
#include "gui/scrollable_widget.h"

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(const renderer::Size& size);

    void draw() override;
    void layout() override;

    void updateMaxScroll() override;

private:
    std::unique_ptr<LabelWidget> folder_label;
};

}
