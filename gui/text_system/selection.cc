#include "base/numeric/saturation_arithmetic.h"
#include "selection.h"

namespace gui {

Caret& Selection::start() {
    return start_caret;
}

Caret& Selection::end() {
    return end_caret;
}

bool Selection::empty() const {
    return start_caret == end_caret;
}

std::pair<size_t, size_t> Selection::range() const {
    return {std::min(start_caret, end_caret).index, std::max(start_caret, end_caret).index};
}

void Selection::setIndex(size_t index, bool extend) {
    end_caret.index = index;
    if (!extend) {
        start_caret = end_caret;
    }
}

void Selection::setRange(size_t start_index, size_t end_index) {
    start_caret.index = start_index;
    end_caret.index = end_index;
}

void Selection::incrementIndex(size_t inc, bool extend) {
    end_caret.index += inc;
    if (!extend) {
        start_caret = end_caret;
    }
}

void Selection::decrementIndex(size_t dec, bool extend) {
    end_caret.index = base::sub_sat(end_caret.index, dec);
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

}
