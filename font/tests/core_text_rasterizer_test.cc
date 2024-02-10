#include "font/core_text_rasterizer.h"
#include "gtest/gtest.h"
#include <iostream>
#include <vector>

TEST(GlyphIndexTest, Alphabet) {
    CoreTextRasterizer ct_rasterizer;
    ct_rasterizer.setup("Arial", 32);

    std::vector<const char*> utf8_strs = {
        u8"a",
        u8"∆",
        u8"😄",
        u8"🥲",
        u8"",
        u8"👨‍👩‍👦",
        u8"👩‍👩‍👧‍👦",
        u8"🇺🇸",
        u8"🏴‍☠️",
    };
    for (const auto& str : utf8_strs) {
        EXPECT_GT(ct_rasterizer.getGlyphIndex(str), 0);
    }
}

TEST(CoreTextRasterizerTest, Metrics) {
    CoreTextRasterizer ct_rasterizer;
    ct_rasterizer.setup("Arial", 32);

    std::vector<const char*> utf8_strs = {
        u8"a",
        u8"∆",
        u8"",
    };
    for (const auto& str : utf8_strs) {
        RasterizedGlyph glyph = ct_rasterizer.rasterizeUTF8(str);
        EXPECT_FALSE(glyph.colored);
        EXPECT_EQ(glyph.buffer.size(), glyph.width * glyph.height * 3);
    }

    std::vector<const char*> utf8_emojis = {
        u8"😄",
        u8"🥲",
        u8"👨‍👩‍👦",
        u8"👩‍👩‍👧‍👦",
        u8"🇺🇸",
        u8"🏴‍☠️",
    };
    for (const auto& str : utf8_emojis) {
        RasterizedGlyph glyph = ct_rasterizer.rasterizeUTF8(str);
        EXPECT_TRUE(glyph.colored);
        EXPECT_EQ(glyph.buffer.size(), glyph.width * glyph.height * 4);
    }
}
