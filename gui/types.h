#pragma once

#include <limits>

namespace gui {

enum class MoveBy {
    kCharacters,
    kLines,
    kWords,
};

enum class MoveTo {
    kBOL,
    kEOL,
    kHardBOL,
    kHardEOL,
    kBOF,
    kEOF,
};

enum class ClickType {
    kSingleClick,
    kDoubleClick,
    kTripleClick,
};

constexpr ClickType click_type_from_count(long click_count) {
    if (click_count == 1) {
        return ClickType::kSingleClick;
    } else if (click_count == 2) {
        return ClickType::kDoubleClick;
    } else {
        return ClickType::kTripleClick;
    }
}

enum class CursorStyle {
    kArrow,
    kIBeam,
    kResizeLeftRight,
    kResizeUpDown,
};

struct Point {
    int x, y;

    friend Point operator+(const Point& p1, const Point& p2) {
        return {p1.x + p2.x, p1.y + p2.y};
    }

    Point& operator+=(const Point& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    friend Point operator-(const Point& p1, const Point& p2) {
        return {p1.x - p2.x, p1.y - p2.y};
    }

    Point& operator-=(const Point& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    friend constexpr Point operator*(const Point& p, int val) {
        return {p.x * val, p.y * val};
    }

    friend constexpr Point operator*(int val, const Point& p) {
        return p * val;
    }

    constexpr Point& operator*=(int val) {
        x *= val;
        y *= val;
        return *this;
    }
};

struct Delta {
    int dx, dy;

    friend constexpr Delta operator*(const Delta& d, int val) {
        return {d.dx * val, d.dy * val};
    }

    friend constexpr Delta operator*(int val, const Delta& d) {
        return d * val;
    }

    constexpr Delta& operator*=(int val) {
        dx *= val;
        dy *= val;
        return *this;
    }
};

struct Size {
    int width;
    int height;

    friend Size operator+(const Size& s1, const Size& s2) {
        return {s1.width + s2.width, s1.height + s2.height};
    }

    Size& operator+=(const Size& rhs) {
        width += rhs.width;
        height += rhs.height;
        return *this;
    }

    friend Size operator-(const Size& s1, const Size& s2) {
        return {s1.width - s2.width, s1.height - s2.height};
    }

    Size& operator-=(const Size& rhs) {
        width -= rhs.width;
        height -= rhs.height;
        return *this;
    }

    constexpr Size& operator*=(int val) {
        width *= val;
        height *= val;
        return *this;
    }

    static constexpr Size minValue() {
        return {};
    }

    static constexpr Size maxValue() {
        return {
            .width = std::numeric_limits<int>::max(),
            .height = std::numeric_limits<int>::max(),
        };
    }
};

constexpr Point operator+(const Point& p, const Size& s) {
    return {p.x + s.width, p.y + s.height};
}

}  // namespace gui
