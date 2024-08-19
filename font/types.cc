#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "types.h"

namespace font {

LineLayout::const_iterator LineLayout::begin() const {
    return {*this, 0, 0};
}

LineLayout::const_iterator LineLayout::end() const {
    return {*this, runs.size(), 0};
}

LineLayout::ConstIterator::ConstIterator(const LineLayout& layout,
                                         size_t run_index,
                                         size_t run_glyph_index)
    : layout{layout}, run_index{run_index}, run_glyph_index{run_glyph_index} {}

LineLayout::ConstIterator::reference LineLayout::ConstIterator::operator*() const {
    return layout.runs[run_index].glyphs[run_glyph_index];
}

LineLayout::ConstIterator::pointer LineLayout::ConstIterator::operator->() {
    return &layout.runs[run_index].glyphs[run_glyph_index];
}

LineLayout::ConstIterator& LineLayout::ConstIterator::operator++() {
    ++run_glyph_index;
    if (run_glyph_index == layout.runs[run_index].glyphs.size()) {
        run_glyph_index = 0;
        ++run_index;
    }
    return *this;
}

LineLayout::ConstIterator LineLayout::ConstIterator::operator++(int) {
    ConstIterator tmp = *this;
    ++(*this);
    return tmp;
}

LineLayout::ConstIterator& LineLayout::ConstIterator::operator--() {
    if (run_glyph_index == 0 && run_index > 0) {
        --run_index;
        run_glyph_index = base::sub_sat(layout.runs[run_index].glyphs.size(), 1_Z);
    } else if (run_glyph_index > 0) {
        --run_glyph_index;
    }
    return *this;
}

LineLayout::ConstIterator LineLayout::ConstIterator::operator--(int) {
    ConstIterator tmp = *this;
    --(*this);
    return tmp;
}

}
