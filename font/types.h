#pragma once

#include <cstdint>
#include <vector>

namespace font {

// TODO: Consider unnesting these inner classes.
struct RasterizedGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    int32_t advance;
    std::vector<uint8_t> buffer;
};

// TODO: Consider making a global "geometry" namespace and moving Point there.
struct Point {
    int x;
    int y;
};

struct ShapedGlyph {
    uint32_t glyph_id;
    Point position;
    Point advance;
    size_t index;  // UTF-8 index in the original text.
};

struct ShapedRun {
    size_t font_id;
    std::vector<ShapedGlyph> glyphs;
};

struct LineLayout {
    int width;
    size_t length;
    std::vector<ShapedRun> runs;

    std::pair<size_t, int> closestForX(int x) const;
    std::pair<size_t, int> closestForIndex(size_t index) const;
    std::pair<size_t, int> prevClosestForIndex(size_t index) const;
    std::pair<size_t, int> nextClosestForIndex(size_t index) const;

    struct ConstIterator;
    using const_iterator = ConstIterator;

    const_iterator begin() const;
    const_iterator end() const;
};

struct LineLayout::ConstIterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const ShapedGlyph;
    using pointer = const ShapedGlyph*;
    using reference = const ShapedGlyph&;

    reference operator*() const;
    pointer operator->();
    ConstIterator& operator++();
    ConstIterator operator++(int);
    ConstIterator& operator--();
    ConstIterator operator--(int);

    friend constexpr bool operator==(const ConstIterator& a, const ConstIterator& b) {
        return a.run_index == b.run_index && a.run_glyph_index == b.run_glyph_index;
    }

    friend constexpr bool operator!=(const ConstIterator& a, const ConstIterator& b) {
        return !(operator==(a, b));
    }

private:
    friend class LineLayout;

    ConstIterator(const LineLayout& layout, size_t run_index, size_t run_glyph_index);

    const LineLayout& layout;
    size_t run_index;
    size_t run_glyph_index;
};

static_assert(std::is_trivially_copy_constructible_v<LineLayout::ConstIterator>);

}
