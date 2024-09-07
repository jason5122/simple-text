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

void Selection::setIndex(size_t index, bool extend) {
    end_caret.index = index;
    if (!extend) {
        start_caret = end_caret;
    }
}

std::pair<size_t, size_t> Selection::range() const {
    return {std::min(start_caret, end_caret).index, std::max(start_caret, end_caret).index};
}

}
