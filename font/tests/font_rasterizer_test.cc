#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include <gtest/gtest.h>

namespace {

#if BUILDFLAG(IS_MAC)
const std::string kOSFontFace = "Menlo";
#elif BUILDFLAG(IS_LINUX)
const std::string kOSFontFace = "Monospace";
#elif BUILDFLAG(IS_WIN)
const std::string kOSFontFace = "Consolas";
#endif

}

namespace font {

TEST(FontRasterizerTest, LayoutLine1) {
    FontRasterizer rasterizer{kOSFontFace, 32};

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(line);

    EXPECT_GT(layout.width, 0);
    EXPECT_EQ(layout.runs.size(), 3_Z);

    const auto& emoji_glyphs = layout.runs[1].glyphs;
    EXPECT_EQ(emoji_glyphs.size(), 2_Z);
    EXPECT_EQ(emoji_glyphs[0].index, 5_Z);
    EXPECT_EQ(emoji_glyphs[1].index, 9_Z);

    const auto& hi_glyphs = layout.runs[2].glyphs;
    EXPECT_EQ(hi_glyphs.size(), 2_Z);
    EXPECT_EQ(hi_glyphs[0].index, 13_Z);
    EXPECT_EQ(hi_glyphs[1].index, 14_Z);

    // Emojis should be colored and should have 4 channels.
    size_t emoji_font_id = layout.runs[1].font_id;
    for (const auto& glyph : emoji_glyphs) {
        auto rglyph = rasterizer.rasterizeUTF8(emoji_font_id, glyph.glyph_id);
        EXPECT_TRUE(rglyph.colored);
        EXPECT_EQ(rglyph.buffer.size(), rglyph.width * rglyph.height * 4_Z);
    }

    // Regular text should not be colored and should have 3 channels.
    size_t monospace_font_id = layout.runs[2].font_id;
    for (const auto& glyph : hi_glyphs) {
        auto rglyph = rasterizer.rasterizeUTF8(monospace_font_id, glyph.glyph_id);
        EXPECT_FALSE(rglyph.colored);
        EXPECT_EQ(rglyph.buffer.size(), rglyph.width * rglyph.height * 3_Z);
    }

    // Ensure positions are monotonically increasing.
    int prev_x = 0;
    for (const auto& run : layout.runs) {
        for (const auto& glyph : run.glyphs) {
            EXPECT_GE(glyph.position.x, prev_x);
            prev_x = std::max(glyph.position.x, prev_x);
        }
    }
}

TEST(FontRasterizerTest, LayoutLine2) {
    FontRasterizer rasterizer{kOSFontFace, 32};

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(line);

    int total_advance = 0;
    for (const auto& run : layout.runs) {
        for (const auto& glyph : run.glyphs) {
            total_advance += glyph.advance.x;
        }
    }

    EXPECT_EQ(total_advance, layout.width);
}

TEST(FontRasterizerTest, LineLayoutClosestForX) {
    FontRasterizer rasterizer{kOSFontFace, 32};

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(line);

    size_t prev_glyph_index = 0;
    int prev_glyph_x = 0;
    for (int x = 0; x < layout.width; ++x) {
        auto [glyph_index, glyph_x] = layout.closestForX(x);

        EXPECT_GE(glyph_index, prev_glyph_index);
        EXPECT_GE(glyph_x, prev_glyph_x);

        prev_glyph_index = glyph_index;
        prev_glyph_x = glyph_x;
    }

    auto [index, x] = layout.closestForX(99999);
    EXPECT_EQ(index, layout.length);
    EXPECT_EQ(x, layout.width);

    std::tie(index, x) = layout.closestForX(0);
    EXPECT_EQ(index, 0_Z);
    EXPECT_EQ(x, 0);
}

TEST(FontRasterizerTest, LineLayoutClosestForIndex) {
    FontRasterizer rasterizer{kOSFontFace, 32};

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(line);

    size_t prev_glyph_index = 0;
    int prev_glyph_x = 0;
    for (size_t index = 0; index < line.length(); ++index) {
        auto [glyph_index, glyph_x] = layout.closestForIndex(index);

        EXPECT_GE(glyph_index, prev_glyph_index);
        EXPECT_GE(glyph_x, prev_glyph_x);

        prev_glyph_index = glyph_index;
        prev_glyph_x = glyph_x;
    }

    auto [index, x] = layout.closestForIndex(99999);
    EXPECT_EQ(index, layout.length);
    EXPECT_EQ(x, layout.width);

    std::tie(index, x) = layout.closestForIndex(0);
    EXPECT_EQ(index, 0_Z);
    EXPECT_EQ(x, 0);
}

}
