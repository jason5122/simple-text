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
    int ascent;
    int descent;
    int font_size;
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
    size_t font_id;
    uint32_t glyph_id;
    Point position;
    Point advance;
    size_t index;  // UTF-8 index in the original text.
};

struct LineLayout {
    size_t layout_font_id;
    int width;
    size_t length;
    std::vector<ShapedGlyph> glyphs;
};

}  // namespace font
