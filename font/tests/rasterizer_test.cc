#include "font/rasterizer.h"
#include "gtest/gtest.h"
#include <iostream>

TEST(GlyphIndexTest, Alphabet) {
    Rasterizer rasterizer;
    rasterizer.setup("Source Code Pro", 32);

    std::cerr << rasterizer.getGlyphIndex("\x61") << '\n';
    std::cerr << rasterizer.getGlyphIndex("\xE2\x88\x86") << '\n';
    std::cerr << rasterizer.getGlyphIndex("\xF0\x9F\x98\x84") << '\n';
    std::cerr << rasterizer.getGlyphIndex("\xF0\x9F\xA5\xB2") << '\n';

    std::cerr << rasterizer.getGlyphIndex("\xEF\xA3\xBF") << '\n';
}

TEST(CoreTextRasterizerTest, Metrics) {
    Rasterizer rasterizer;
    rasterizer.setup("Source Code Pro", 32);

    RasterizedGlyph glyph = rasterizer.rasterizeUTF8(u8"a");

    EXPECT_FALSE(glyph.colored);
    EXPECT_EQ(glyph.buffer.size(), glyph.width * glyph.height * 3);
}
