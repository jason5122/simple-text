#include <gtest/gtest.h>

#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include "util/random_util.h"

namespace font {

static_assert(!std::is_copy_constructible_v<FontRasterizer>);
static_assert(!std::is_copy_assignable_v<FontRasterizer>);
static_assert(!std::is_move_constructible_v<FontRasterizer>);
static_assert(!std::is_move_assignable_v<FontRasterizer>);

static_assert(std::is_trivially_copy_constructible_v<LineLayout::ConstIterator>);
static_assert(std::is_trivially_copy_assignable_v<LineLayout::ConstIterator>);

TEST(FontRasterizerTest, LayoutLine1) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(font_id, line);

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
        auto rglyph = rasterizer.rasterize(emoji_font_id, glyph.glyph_id);
        EXPECT_TRUE(rglyph.colored);
        EXPECT_EQ(rglyph.buffer.size(), rglyph.width * rglyph.height * 4_Z);
    }

    // Regular text should not be colored, but should also have 4 channels.
    size_t monospace_font_id = layout.runs[2].font_id;
    for (const auto& glyph : hi_glyphs) {
        auto rglyph = rasterizer.rasterize(monospace_font_id, glyph.glyph_id);
        EXPECT_FALSE(rglyph.colored);
        EXPECT_EQ(rglyph.buffer.size(), rglyph.width * rglyph.height * 4_Z);
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
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(font_id, line);

    int total_advance = 0;
    for (const auto& run : layout.runs) {
        for (const auto& glyph : run.glyphs) {
            total_advance += glyph.advance.x;
        }
    }

    EXPECT_EQ(total_advance, layout.width);
}

// We should move the rasterized bitmap data *directly* into OpenGL. Manually creating the buffer
// via rearranging pixels will be too slow.
TEST(FontRasterizerTest, RasterizePerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);

    auto layout = rasterizer.layoutLine(font_id, "a");
    uint32_t glyph_id = layout.runs[0].glyphs[0].glyph_id;

    for (int i = 0; i < 10000; ++i) {
        auto rglyph = rasterizer.rasterize(font_id, glyph_id);
        EXPECT_GT(rglyph.width, 0);
        EXPECT_GT(rglyph.height, 0);
    }
}

TEST(FontRasterizerTest, LineLayoutPerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);

    for (int i = 0; i < 1000; ++i) {
        std::string str = util::RandomString(100);
        auto layout = rasterizer.layoutLine(font_id, str);
    }
}

}  // namespace font
