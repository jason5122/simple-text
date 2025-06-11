#pragma once

#include "base/numeric/saturation_arithmetic.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

// References:
// https://github.com/zed-industries/zed/blob/40ecc38dd25ffdec4deb6e27ee91b72e85a019eb/crates/text/src/selection.rs

namespace gui {

// TODO: Remove methods; make this a pure struct.
struct Selection {
    size_t start = 0;
    size_t end = 0;

    constexpr bool empty() const;
    constexpr size_t length() const;
    constexpr std::pair<size_t, size_t> range() const;

    constexpr void set_index(size_t index, bool extend);
    constexpr void set_range(size_t start_index, size_t end_index);
    constexpr void increment(size_t inc, bool extend);
    constexpr void decrement(size_t dec, bool extend);
    constexpr void collapse_left();
    constexpr void collapse_right();
};

constexpr bool Selection::empty() const { return start == end; }

constexpr size_t Selection::length() const {
    auto [left, right] = range();
    assert(right >= left);
    return right - left;
}

constexpr std::pair<size_t, size_t> Selection::range() const {
    return {std::min(start, end), std::max(start, end)};
}

constexpr void Selection::set_index(size_t index, bool extend) {
    end = index;
    if (!extend) {
        start = end;
    }
}

constexpr void Selection::set_range(size_t start_index, size_t end_index) {
    start = start_index;
    end = end_index;
}

constexpr void Selection::increment(size_t inc, bool extend) {
    end += inc;
    if (!extend) {
        start = end;
    }
}

constexpr void Selection::decrement(size_t dec, bool extend) {
    end = base::sub_sat(end, dec);
    if (!extend) {
        start = end;
    }
}

constexpr void Selection::collapse_left() {
    auto [start, _] = range();
    this->start = start;
    this->end = start;
}

constexpr void Selection::collapse_right() {
    auto [_, end] = range();
    this->start = end;
    this->end = end;
}

}  // namespace gui
