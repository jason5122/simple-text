#pragma once

#include "gui/renderer/renderer.h"
#include "gui/widget/widget.h"

namespace gui {

class FindPanelWidget : public Widget {
public:
    FindPanelWidget(const app::Size& size);

    void draw() override;

    std::string_view className() const override {
        return "FindWidget";
    }

private:
    // static constexpr Rgba kFindPanelColor{199, 203, 209, 255};  // Light.
    // static constexpr Rgba kFindPanelColor{46, 50, 56, 255};  // Dark.
    static constexpr Rgba kFindPanelColor{255, 0, 0, 255};  // TODO: Debug use; remove this.
    static constexpr Rgba kTextInputColor{255, 255, 255, 255};
};

}  // namespace gui
