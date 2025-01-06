#pragma once

#include "gui/renderer/renderer.h"
#include "gui/widget/scrollable_widget.h"

#include <vector>

namespace gui {

class AtlasWidget : public ScrollableWidget {
public:
    AtlasWidget();

    void draw() override;
    void updateMaxScroll() override;

    constexpr std::string_view className() const final override {
        return "AtlasWidget";
    }

private:
    // static constexpr Rgb kSideBarColor{235, 237, 239};  // Light.
    static constexpr Rgb kSideBarColor{34, 38, 42};  // Dark.

    // TODO: Debug use; remove this.
    std::vector<Rgb> page_colors;
};

}  // namespace gui
