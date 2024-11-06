#pragma once

#include <cstdint>
#include <vector>

namespace font {

enum class FontStyle {
    kNone = 0,
    kBold = 1 << 0,
    kItalic = 1 << 1,
};

inline constexpr auto operator|(FontStyle l, FontStyle r) {
    using U = std::underlying_type_t<FontStyle>;
    return static_cast<FontStyle>(static_cast<U>(l) | static_cast<U>(r));
}

inline constexpr auto& operator|=(FontStyle& l, FontStyle r) {
    return l = l | r;
}

inline constexpr auto operator&(FontStyle l, FontStyle r) {
    using U = std::underlying_type_t<FontStyle>;
    return static_cast<FontStyle>(static_cast<U>(l) & static_cast<U>(r));
}

inline constexpr auto& operator&=(FontStyle& l, FontStyle r) {
    return l = l & r;
}

inline constexpr auto operator^(FontStyle l, FontStyle r) {
    using U = std::underlying_type_t<FontStyle>;
    return static_cast<FontStyle>(static_cast<U>(l) ^ static_cast<U>(r));
}

inline constexpr auto& operator^=(FontStyle& l, FontStyle r) {
    return l = l ^ r;
}

inline constexpr auto operator~(FontStyle m) {
    using U = std::underlying_type_t<FontStyle>;
    return static_cast<FontStyle>(~static_cast<U>(m));
}

struct Metrics {
    int line_height;
    int descent;
    int ascent;
};

struct RasterizedGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
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
    size_t length;
};

struct ShapedRun {
    size_t font_id;
    std::vector<ShapedGlyph> glyphs;
};

struct LineLayout {
    size_t layout_font_id;
    int width;
    size_t length;
    std::vector<ShapedRun> runs;
    int ascent;

    struct ConstIterator;
    using const_iterator = ConstIterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    const_iterator begin() const;
    const_iterator end() const;
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;
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
    friend struct LineLayout;

    ConstIterator(const LineLayout* layout, size_t run_index, size_t run_glyph_index);

    const LineLayout* layout;
    size_t run_index;
    size_t run_glyph_index;
};

static_assert(std::is_trivially_copy_constructible_v<LineLayout::ConstIterator>);
static_assert(std::is_trivially_copy_assignable_v<LineLayout::ConstIterator>);

}  // namespace font
