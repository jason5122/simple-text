#pragma once

#include "gui/widget/widget.h"
#include <chrono>

namespace gui {

class ScrollableWidget : public Widget {
public:
    ScrollableWidget() {}
    ScrollableWidget(const Size& size);

    virtual void updateMaxScroll() = 0;

    void scroll(const Point& mouse_pos, const Point& delta) override;

    Widget* getWidgetAtPosition(const Point& pos) override {
        return hitTest(pos) ? this : nullptr;
    }
    std::string_view getClassName() const override {
        return "ScrollableWidget";
    };

protected:
    Point scroll_offset{};
    Point max_scroll_offset{};

private:
    static constexpr long long kScrollEventSeparation = 28;
    static constexpr float kUnlockLowerBound = 6;
    static constexpr float kUnlockPercent = 1.9;

    enum class ScrollAxis { None, Vertical, Horizontal };

    std::chrono::time_point<std::chrono::system_clock> prev_scroll;
    ScrollAxis axis{ScrollAxis::None};
};

}
