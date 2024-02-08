#include "font/rasterizer.h"
#include "gtest/gtest.h"
#include <iostream>

TEST(GlyphIndexTest, Alphabet) {
    Rasterizer rasterizer;
    rasterizer.setup("Source Code Pro", 16);

    std::cout << rasterizer.getGlyphIndex("\x61") << '\n';
    std::cout << rasterizer.getGlyphIndex("\xE2\x88\x86") << '\n';
    std::cout << rasterizer.getGlyphIndex("\xF0\x9F\x98\x84") << '\n';
    std::cout << rasterizer.getGlyphIndex("\xF0\x9F\xA5\xB2") << '\n';

    std::cout << rasterizer.getGlyphIndex("\xEF\xA3\xBF") << '\n';
}
