#pragma once

#include "base/utf8_string.h"
#include "gui/widget.h"

namespace gui {

class SideBarWidget : public Widget {
public:
    SideBarWidget(const renderer::Size& size);

    void draw() override;
    void scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) override;

private:
    const base::Utf8String kFoldersText{"FOLDERS"};

    renderer::Point scroll_offset{};
};

}
