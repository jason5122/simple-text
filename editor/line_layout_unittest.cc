#include "build/build_config.h"
#include "editor/line_layout.h"
#include "fx/fx.h"
#include <gtest/gtest.h>
#include <memory>

namespace editor {

namespace {

#if BUILDFLAG(IS_MAC)
constexpr std::string_view kMonospaceFamily = "Menlo";
#elif BUILDFLAG(IS_WIN)
constexpr std::string_view kMonospaceFamily = "Consolas";
#else
constexpr std::string_view kMonospaceFamily = "Monospace";
#endif

}  // namespace

// The first glyph's pen is the start of the line regardless of how the backend nudges its ink.
TEST(LineLayoutTest, FirstGlyphStartsAtZero) {
    for (std::string_view family : {std::string_view("system"), kMonospaceFamily}) {
        std::unique_ptr<fx_font> font = fx_create_font(family, 16.0f, 0);
        ASSERT_TRUE(font) << family;
        for (float scale : {1.0f, 2.0f}) {
            const LineLayout layout = layout_line(0, *font, scale, "assert(x);");
            ASSERT_FALSE(layout.glyphs.empty()) << family;
            EXPECT_EQ(layout.glyphs.front().x, 0) << family << " at " << scale << "x";
        }
    }
}

#if BUILDFLAG(IS_MAC)
// Carets, selections and hit testing sit on the pen grid, so every cell of a monospace line must
// have the same width, including the last one. 16pt is the largest size at which the Core Text
// backend snaps advances to whole pixels, and its snapping delta must not leak into the pens.
TEST(LineLayoutTest, MonospaceCellsAreUniform) {
    std::unique_ptr<fx_font> font = fx_create_font(kMonospaceFamily, 16.0f, 0);
    ASSERT_TRUE(font);
    ASSERT_TRUE(font->widths().monospace);

    const LineLayout layout = layout_line(0, *font, 2.0f, "assert(x);");
    ASSERT_EQ(layout.glyphs.size(), 10u);

    const int cell = layout.glyphs.front().advance;
    EXPECT_GT(cell, 0);
    for (size_t i = 0; i < layout.glyphs.size(); ++i) {
        EXPECT_EQ(layout.glyphs[i].x, static_cast<int>(i) * cell) << "glyph " << i;
        EXPECT_EQ(layout.glyphs[i].advance, cell) << "glyph " << i;
    }
    EXPECT_EQ(layout.width, static_cast<int>(layout.glyphs.size()) * cell);
}
#endif

}  // namespace editor
