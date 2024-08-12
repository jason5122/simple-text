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
};

}