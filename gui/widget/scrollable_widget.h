#pragma once

#include "gui/widget/widget.h"
#include <chrono>

namespace gui {

class ScrollableWidget : public Widget {
public:
    ScrollableWidget() {}
    ScrollableWidget(const app::Size& size);

    virtual void updateMaxScroll() = 0;

    void scroll(const app::Point& mouse_pos, const app::Delta& delta) override;

    constexpr std::string_view className() const override {
        return "ScrollableWidget";
    }

protected:
    app::Point scroll_offset{};
    app::Point max_scroll_offset{};

private:
    static constexpr long long kScrollEventSeparation = 28;
    static constexpr float kUnlockLowerBound = 6;
    static constexpr float kUnlockPercent = 1.9;

    enum class ScrollAxis { None, Vertical, Horizontal };

    std::chrono::time_point<std::chrono::system_clock> prev_scroll;
    ScrollAxis axis{ScrollAxis::None};
};

}  // namespace gui
