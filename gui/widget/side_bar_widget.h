#pragma once

#include "gui/widget/label_widget.h"
#include "gui/widget/scrollable_widget.h"

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(const Size& size);

    void draw() override;
    void layout() override;

    void updateMaxScroll() override;

private:
    static constexpr Rgba kSideBarColor{235, 237, 239, 255};
    static constexpr Rgba kScrollBarColor{190, 190, 190, 255};
    static constexpr Rgba kFolderIconColor{142, 142, 142, 255};

    std::unique_ptr<LabelWidget> folder_label;
};

}
