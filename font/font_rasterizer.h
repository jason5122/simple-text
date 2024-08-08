#pragma once

#include <memory>
#include <string>
#include <vector>

namespace font {

class FontRasterizer {
public:
    FontRasterizer(const std::string& font_name_utf8, int font_size);
    ~FontRasterizer();

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

    RasterizedGlyph rasterizeUTF8(size_t font_id, uint32_t glyph_id) const;
    LineLayout layoutLine(std::string_view str8) const;
    int getLineHeight() const;

    // TODO: Make these private. Currently, we make these public as a hack for DirectWrite.
    class impl;
    std::unique_ptr<impl> pimpl;

private:
    int line_height;
    int descent;
};

}
