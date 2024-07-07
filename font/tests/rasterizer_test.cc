#include "font/font_rasterizer.h"
#include "gtest/gtest.h"
#include <vector>

TEST(RasterizerTest, RasterizedGlyph) {
    font::FontRasterizer rasterizer{"Arial", 32};

    std::vector<const char*> utf8_strs = {
        "a",
        "∆",
        "",
    };
    for (const auto& str : utf8_strs) {
        font::FontRasterizer::RasterizedGlyph glyph = rasterizer.rasterizeUTF8(str);
        EXPECT_FALSE(glyph.colored);
        EXPECT_EQ(glyph.buffer.size(), glyph.width * glyph.height * 3);
    }

    std::vector<const char*> utf8_colored_strs = {
        "😄", "🥲", "👨‍👩‍👦", "👩‍👩‍👧‍👦", "🇺🇸", "🏴‍☠️",
    };
    for (const auto& str : utf8_colored_strs) {
        font::FontRasterizer::RasterizedGlyph glyph = rasterizer.rasterizeUTF8(str);
        EXPECT_TRUE(glyph.colored);
        EXPECT_EQ(glyph.buffer.size(), glyph.width * glyph.height * 4);
    }
}
