#pragma once

#include "base/utf8_string.h"
#include "gui/label_widget.h"
#include "gui/scrollable_widget.h"

namespace gui {

class SideBarWidget : public ScrollableWidget {
public:
    SideBarWidget(const renderer::Size& size);

    void draw() override;

    void updateMaxScroll() override;

private:
    // const base::Utf8String kFoldersText{"FOLDERS"};
    std::unique_ptr<LabelWidget> folder_label;
};

}
