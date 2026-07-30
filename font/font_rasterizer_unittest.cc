#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include <gtest/gtest.h>

#if BUILDFLAG(IS_WIN)
#include "base/win/scoped_com_initializer.h"
#include <memory>
#endif

namespace font {

// FontRasterizer::instance() constructs WIC/Direct2D/DirectWrite factories,
// which require COM to be initialized on the calling thread. Bracket the suite
// so the apartment is up for exactly these tests and no others.
class FontRasterizerTest : public ::testing::Test {
#if BUILDFLAG(IS_WIN)
protected:
    static void SetUpTestSuite() {
        com_ = std::make_unique<base::win::ScopedCOMInitializer>();
    }
    static void TearDownTestSuite() { com_.reset(); }

    static std::unique_ptr<base::win::ScopedCOMInitializer> com_;
#endif
};

#if BUILDFLAG(IS_WIN)
std::unique_ptr<base::win::ScopedCOMInitializer> FontRasterizerTest::com_;
#endif

TEST_F(FontRasterizerTest, LayoutLine1) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.add_system_font(32);
    auto layout = rasterizer.layout_line(font_id, "Hello😄🙂hi");

    EXPECT_GT(layout.width, 0);
    EXPECT_EQ(layout.glyphs.size(), size_t{9});

    auto& letter_glyph_1 = layout.glyphs[0];
    auto& letter_glyph_2 = layout.glyphs[1];
    auto& emoji_glyph_1 = layout.glyphs[5];
    auto& emoji_glyph_2 = layout.glyphs[6];

    EXPECT_EQ(letter_glyph_1.index, size_t{0});
    EXPECT_EQ(letter_glyph_2.index, size_t{1});
    EXPECT_EQ(emoji_glyph_1.index, size_t{5});
    EXPECT_EQ(emoji_glyph_2.index, size_t{9});

    // Emojis should be colored and should have 4 channels.
    auto emoji_rglyph = rasterizer.rasterize(emoji_glyph_1.font_id, emoji_glyph_1.glyph_id);
    EXPECT_TRUE(emoji_rglyph.colored);
    EXPECT_EQ(emoji_rglyph.buffer.size(), emoji_rglyph.width * emoji_rglyph.height * 4);

    // Regular text should not be colored, but should also have 4 channels.
    auto letter_rglyph = rasterizer.rasterize(letter_glyph_1.font_id, letter_glyph_1.glyph_id);
    EXPECT_FALSE(letter_rglyph.colored);
    EXPECT_EQ(letter_rglyph.buffer.size(), letter_rglyph.width * letter_rglyph.height * 4);

    // Ensure positions are monotonically increasing.
    int prev_x = 0;
    for (const auto& glyph : layout.glyphs) {
        EXPECT_GE(glyph.position.x, prev_x);
        prev_x = std::max(glyph.position.x, prev_x);
    }
}

TEST_F(FontRasterizerTest, LayoutLine2) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.add_system_font(32);
    auto layout = rasterizer.layout_line(font_id, "Hello😄🙂hi");

    int total_advance = 0;
    for (const auto& glyph : layout.glyphs) {
        total_advance += glyph.advance.x;
    }

    EXPECT_EQ(total_advance, layout.width);
}

}  // namespace font
