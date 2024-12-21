#include "selection.h"

#include "base/numeric/saturation_arithmetic.h"

#include <algorithm>

namespace gui {

size_t& Selection::start() {
    return start_caret;
}

size_t& Selection::end() {
    return end_caret;
}

bool Selection::empty() const {
    return start_caret == end_caret;
}

std::pair<size_t, size_t> Selection::range() const {
    return {std::min(start_caret, end_caret), std::max(start_caret, end_caret)};
}

void Selection::setIndex(size_t index, bool extend) {
    end_caret = index;
    if (!extend) {
        start_caret = end_caret;
    }
}

void Selection::setRange(size_t start_index, size_t end_index) {
    start_caret = start_index;
    end_caret = end_index;
}

void Selection::increment(size_t inc, bool extend) {
    end_caret += inc;
    if (!extend) {
        start_caret = end_caret;
    }
}

void Selection::decrement(size_t dec, bool extend) {
    end_caret = base::sub_sat(end_caret, dec);
    if (!extend) {
        start_caret = end_caret;
    }
}

void Selection::collapse(Direction direction) {
    auto [start, end] = range();
    if (direction == Direction::kLeft) {
        setIndex(start, false);
    }
    if (direction == Direction::kRight) {
        setIndex(end, false);
    }
}

}  // namespace gui
