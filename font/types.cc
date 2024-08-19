#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "types.h"

// TODO: Debug use; remove this.
#include <format>
#include <iostream>

namespace font {

std::pair<size_t, int> LineLayout::closestForIndex(size_t index, bool exclude_end) const {
    for (auto it = begin(); it != end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(end())) {
            return {glyph.index, glyph.position.x};
        }

        if (glyph.index >= index) {
            return {glyph.index, glyph.position.x};
        }
    }
    return {length, width};
}

std::pair<size_t, int> LineLayout::prevClosestForIndex(size_t index) const {
    for (auto it = begin(); it != end(); ++it) {
        const auto& glyph = *it;

        if (glyph.index >= index) {
            // TODO: Replace this with saturating sub for iterators.
            if (it != begin()) --it;

            return {(*it).index, (*it).position.x};
        }
    }
    auto it = end();
    // TODO: Replace this with saturating sub for iterators.
    if (it != begin()) --it;

    return {(*it).index, (*it).position.x};
}

std::pair<size_t, int> LineLayout::nextClosestForIndex(size_t index, bool exclude_end) const {
    for (auto it = begin(); it != end(); ++it) {
        const auto& glyph = *it;

        // Exclude end if requested.
        if (exclude_end && it == std::prev(end())) {
            return {glyph.index, glyph.position.x};
        }

        if (glyph.index >= index) {
            ++it;

            if (it == end()) {
                return {length, width};
            } else {
                return {(*it).index, (*it).position.x};
            }
        }
    }
    return {length, width};
}

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
