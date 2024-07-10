#pragma once

#include "gui/widget/widget.h"

namespace gui {

class ScrollableWidget : public Widget {
public:
    ScrollableWidget() {}
    ScrollableWidget(const renderer::Size& size);

    virtual void updateMaxScroll() = 0;

    void scroll(const renderer::Point& mouse_pos, const renderer::Point& delta) override;

protected:
    renderer::Point scroll_offset{};
    renderer::Point max_scroll_offset{};

private:
    static constexpr long long kScrollEventSeparation = 28;
    static constexpr float kUnlockLowerBound = 6;
    static constexpr float kUnlockPercent = 1.9;

    enum class ScrollAxis { None, Vertical, Horizontal };

    std::chrono::time_point<std::chrono::system_clock> prev_scroll;
    ScrollAxis axis{ScrollAxis::None};
};

}
