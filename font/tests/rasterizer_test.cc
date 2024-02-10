#include "font/core_text_rasterizer.h"
#include "gtest/gtest.h"
#include <iostream>

TEST(GlyphIndexTest, Alphabet) {
    CoreTextRasterizer ct_rasterizer;
    ct_rasterizer.setup("Source Code Pro", 32);

    std::cerr << ct_rasterizer.getGlyphIndex("\x61") << '\n';
    std::cerr << ct_rasterizer.getGlyphIndex("\xE2\x88\x86") << '\n';
    std::cerr << ct_rasterizer.getGlyphIndex("\xF0\x9F\x98\x84") << '\n';
    std::cerr << ct_rasterizer.getGlyphIndex("\xF0\x9F\xA5\xB2") << '\n';

    std::cerr << ct_rasterizer.getGlyphIndex("\xEF\xA3\xBF") << '\n';
}

TEST(CoreTextRasterizerTest, Metrics) {
    CoreTextRasterizer ct_rasterizer;
    ct_rasterizer.setup("Source Code Pro", 32);

    RasterizedGlyph glyph = ct_rasterizer.rasterizeUTF8(u8"a");

    EXPECT_FALSE(glyph.colored);
    EXPECT_EQ(glyph.buffer.size(), glyph.width * glyph.height * 3);
}
