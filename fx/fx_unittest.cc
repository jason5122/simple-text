#include "base/files/file_path.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "fx/fx.h"
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#if BUILDFLAG(IS_WIN)
#include "base/strings.h"
#include "base/unicode.h"
#endif

namespace {

// Ahem is the CSS working group's test font: 1000 units per em, ascent 800, descent 200 and no
// line gap. Every letter is a filled em square, `p` is only the part below the baseline, `É` only
// the part above it, and the space has no outline. At 20px every metric is a whole pixel, which
// keeps each backend's own rounding out of the expectations below, with one exception.
constexpr float kSize = 20.0f;
// Core Text keeps normalized metrics in 16.16 fixed point, so Ahem's 0.8 ascent comes back as
// 52429/65536 and the backend's ceil lifts an exact 16 to 17. Its 0.2 descent rounds the other
// way and survives. DirectWrite and Pango report the design ratios exactly.
#if BUILDFLAG(IS_MAC)
constexpr float kAscent = 17.0f;
#else
constexpr float kAscent = 16.0f;
#endif
constexpr float kDescent = 4.0f;
constexpr float kLineHeight = kAscent + kDescent;

std::string ahem_path() {
    base::FilePath exe;
    if (!base::PathService::get(base::PathKey::kFileExe, &exe)) return {};
    const base::FilePath path = exe.DirName()
                                    .Append(FILE_PATH_LITERAL("test_fonts"))
                                    .Append(FILE_PATH_LITERAL("Ahem.ttf"));
#if BUILDFLAG(IS_WIN)
    // TODO: Consider making `base::FilePath` UTF-8 on Windows.
    return base::utf16_to_utf8(base::wide_to_utf16(path.value()));
#else
    return path.value();
#endif
}

std::unique_ptr<fx_font> load_ahem(uint32_t attrs = 0) {
    return fx_create_font_from_file(ahem_path(), kSize, attrs);
}

uint32_t face_of(const fx_glyph& glyph) { return glyph.id >> 16; }

uint32_t index_of(const fx_glyph& glyph) { return glyph.id & 0xffff; }

TEST(FxFontFileTest, LoadsTheFile) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font) << ahem_path();
    EXPECT_EQ(font->attrs(), 0u);
    EXPECT_FALSE(fx_create_font_from_file(ahem_path() + ".missing", kSize, 0));
}

TEST(FxFontFileTest, MetricsFollowTheDesignRatios) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font);
    const fx_font_metrics metrics = font->metrics();
    EXPECT_EQ(metrics.ascent, kAscent);
    EXPECT_EQ(metrics.descent, kDescent);
    EXPECT_EQ(metrics.leading, 0.0f);
    EXPECT_EQ(metrics.line_height, kLineHeight);
    EXPECT_EQ(font->raster_ascent(), kAscent);

    const fx_font_widths widths = font->widths();
    EXPECT_EQ(widths.em_width, kSize);
    EXPECT_TRUE(widths.monospace);
}

TEST(FxFontFileTest, ShapesOneSquarePerCharacter) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font);
    const std::unique_ptr<fx_layout> layout = font->shape("ABC");
    ASSERT_TRUE(layout);
    EXPECT_EQ(layout->advance, 3 * kSize);
    EXPECT_EQ(layout->line_height, kLineHeight);
    ASSERT_EQ(layout->glyphs.size(), 3u);
    for (size_t i = 0; i < layout->glyphs.size(); ++i) {
        const fx_glyph& glyph = layout->glyphs[i];
        EXPECT_EQ(face_of(glyph), 0u) << i;
        // Glyph ids are the file's own indices, which pins the face to the file rather than to an
        // installed font of the same name. A, B and C are consecutive in Ahem's glyph order.
        EXPECT_EQ(index_of(glyph), 35u + i) << i;
        EXPECT_EQ(glyph.x_offset, static_cast<float>(i) * kSize) << i;
        EXPECT_EQ(glyph.y_offset, 0.0f) << i;
        EXPECT_EQ(glyph.cluster, static_cast<uint32_t>(i)) << i;
    }
}

TEST(FxFontFileTest, ClustersAreUtf8ByteOffsets) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font);
    // U+00E9 is two bytes in UTF-8, so B starts at byte 3 whichever encoding supplied the text.
    auto check = [](const std::unique_ptr<fx_layout>& layout, const char* encoding) {
        ASSERT_TRUE(layout) << encoding;
        ASSERT_EQ(layout->glyphs.size(), 3u) << encoding;
        EXPECT_EQ(layout->glyphs[0].cluster, 0u) << encoding;
        EXPECT_EQ(layout->glyphs[1].cluster, 1u) << encoding;
        EXPECT_EQ(layout->glyphs[2].cluster, 3u) << encoding;
        EXPECT_EQ(layout->advance, 3 * kSize) << encoding;
    };
    check(font->shape("AéB"), "utf8");
    check(font->shape(U"AéB"), "utf32");
}

TEST(FxFontFileTest, MissingCharactersFallBackToAnotherFace) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font);
    // Cyrillic is outside Ahem's cmap, so the middle glyph comes from a system face registered
    // after the file's own. Which face that is depends on the platform; that it is not face 0
    // does not.
    const std::unique_ptr<fx_layout> layout = font->shape("AЯB");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 3u);
    EXPECT_EQ(face_of(layout->glyphs[0]), 0u);
    EXPECT_NE(face_of(layout->glyphs[1]), 0u);
    EXPECT_NE(index_of(layout->glyphs[1]), 0u);
    EXPECT_EQ(face_of(layout->glyphs[2]), 0u);
    EXPECT_EQ(layout->glyphs[0].x_offset, 0.0f);
    EXPECT_EQ(layout->glyphs[1].x_offset, kSize);
    EXPECT_EQ(layout->glyphs[1].cluster, 1u);
    EXPECT_EQ(layout->glyphs[2].cluster, 3u);
}

struct InkCase {
    const char* text;
    int height;     // rows of ink at 1x
    int bearing_y;  // top of the ink relative to the baseline at 1x
};

TEST(FxFontFileTest, GlyphCacheCropsToTheInk) {
    // Aliased rendering keeps ClearType and LCD filtering from bleeding coverage past the box.
    std::unique_ptr<fx_font> font = load_ahem(FX_FONT_NO_ANTIALIAS);
    ASSERT_TRUE(font);
    // The ink box comes from the glyph outline rather than from the font metrics, so these hold
    // on macOS too despite the fixed-point ascent above.
    const InkCase cases[] = {
        {"A", 20, -16},  // the full em box: 16px above the baseline, 4px below
        {"p", 4, 0},     // descender only
        {"É", 16, -16},  // ascender only
    };
    for (int scale : {1, 2}) {
        fx_glyph_cache cache(*font, static_cast<float>(scale));
        for (const InkCase& c : cases) {
            SCOPED_TRACE(testing::Message() << c.text << " at " << scale << "x");
            const std::unique_ptr<fx_layout> layout = font->shape(c.text);
            ASSERT_TRUE(layout);
            ASSERT_EQ(layout->glyphs.size(), 1u);
            const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(layout->glyphs[0].id);
            EXPECT_FALSE(data.colored);
            const fx_glyph_cache::glyph_phase& phase = data.phase_at(0);
            ASSERT_NE(phase.pixels, nullptr);
            EXPECT_EQ(static_cast<int>(phase.width), 20 * scale);
            EXPECT_EQ(static_cast<int>(phase.height), c.height * scale);
            EXPECT_EQ(static_cast<int>(phase.bearing_x), 0);
            EXPECT_EQ(static_cast<int>(phase.bearing_y), c.bearing_y * scale);

            // White foreground over the cache's opaque black background: every pixel inside the
            // crop is fully covered.
            size_t uncovered = 0;
            const size_t pixel_count = static_cast<size_t>(phase.width) * phase.height;
            for (size_t i = 0; i < pixel_count; ++i) {
                if (phase.pixels[i] != 0xffffffffu) ++uncovered;
            }
            EXPECT_EQ(uncovered, 0u);
        }

        const std::unique_ptr<fx_layout> space = font->shape(" ");
        ASSERT_TRUE(space);
        ASSERT_EQ(space->glyphs.size(), 1u);
        EXPECT_EQ(cache.lookup_glyph_data(space->glyphs[0].id).phase_at(0).pixels, nullptr)
            << "space at " << scale << "x";
    }
}

TEST(FxFontFileTest, SubpixelPhasesShiftTheInk) {
    if constexpr (fx_glyph_cache::phase_count == 1) {
        GTEST_SKIP() << "this backend positions glyphs on whole pixels";
    }
    std::unique_ptr<fx_font> font = load_ahem(FX_FONT_GRAY_ANTIALIAS);
    ASSERT_TRUE(font);
    fx_glyph_cache cache(*font, 1.0f);
    const std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);
    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(layout->glyphs[0].id);

    // Phase 3 is half a pixel to the right: the square now straddles 21 columns, the two edge
    // columns are partially covered and everything between them is solid.
    const fx_glyph_cache::glyph_phase& half = data.phase_at(3);
    ASSERT_NE(half.pixels, nullptr);
    EXPECT_EQ(static_cast<int>(half.width), 21);
    EXPECT_EQ(static_cast<int>(half.height), 20);
    EXPECT_EQ(static_cast<int>(half.bearing_x), 0);
    EXPECT_EQ(static_cast<int>(half.bearing_y), -16);
    auto green = [&](int x, int y) { return (half.pixels[y * half.width + x] >> 8) & 0xffu; };
    for (int y = 0; y < half.height; ++y) {
        EXPECT_GT(green(0, y), 0u) << "row " << y;
        EXPECT_LT(green(0, y), 255u) << "row " << y;
        EXPECT_GT(green(20, y), 0u) << "row " << y;
        EXPECT_LT(green(20, y), 255u) << "row " << y;
        EXPECT_EQ(green(10, y), 255u) << "row " << y;
    }
}

}  // namespace
