#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/unicode.h"
#include "build/build_config.h"
#include "fx/fx.h"
#include "fx/fx_internal.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#if BUILDFLAG(IS_WIN)
#include "base/strings.h"
#include "fx/win/dwrite_shaping.h"
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

// The fonts //fx:test_fonts copies beside the binary: Ahem and the files generate.py builds.
std::string test_font_path(std::string_view name) {
    base::FilePath exe;
    if (!base::PathService::get(base::PathKey::kFileExe, &exe)) return {};
    base::FilePath path = exe.DirName().Append(FILE_PATH_LITERAL("test_fonts"));
#if BUILDFLAG(IS_WIN)
    // TODO: Consider making `base::FilePath` UTF-8 on Windows.
    std::string directory = base::utf16_to_utf8(base::wide_to_utf16(path.value()));
#else
    std::string directory = path.value();
#endif
    directory += '/';
    directory += name;
    return directory;
}

std::string ahem_path() { return test_font_path("Ahem.ttf"); }

std::unique_ptr<fx_font> load_test_font(std::string_view name,
                                        uint32_t attrs = 0,
                                        float size = kSize) {
    return fx_create_font_from_file(test_font_path(name), size, attrs);
}

std::unique_ptr<fx_font> load_ahem(uint32_t attrs = 0) {
    return load_test_font("Ahem.ttf", attrs);
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
    fx_font_metrics metrics = font->metrics();
    EXPECT_EQ(metrics.ascent, kAscent);
    EXPECT_EQ(metrics.descent, kDescent);
    EXPECT_EQ(metrics.leading, 0.0f);
    EXPECT_EQ(metrics.line_height, kLineHeight);
    EXPECT_EQ(font->raster_ascent(), kAscent);

    fx_font_widths widths = font->widths();
    EXPECT_EQ(widths.em_width, kSize);
    EXPECT_TRUE(widths.monospace);
}

TEST(FxFontFileTest, ShapesOneSquarePerCharacter) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font);
    std::unique_ptr<fx_layout> layout = font->shape("ABC");
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
    std::unique_ptr<fx_layout> layout = font->shape("AЯB");
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
    InkCase cases[] = {
        {"A", 20, -16},  // the full em box: 16px above the baseline, 4px below
        {"p", 4, 0},     // descender only
        {"É", 16, -16},  // ascender only
    };
    for (int scale : {1, 2}) {
        fx_glyph_cache cache(*font, static_cast<float>(scale));
        for (const InkCase& c : cases) {
            SCOPED_TRACE(testing::Message() << c.text << " at " << scale << "x");
            std::unique_ptr<fx_layout> layout = font->shape(c.text);
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
            size_t pixel_count = static_cast<size_t>(phase.width) * phase.height;
            for (size_t i = 0; i < pixel_count; ++i) {
                if (phase.pixels[i] != 0xffffffffu) ++uncovered;
            }
            EXPECT_EQ(uncovered, 0u);
        }

        std::unique_ptr<fx_layout> space = font->shape(" ");
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
    std::unique_ptr<fx_layout> layout = font->shape("A");
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

// Text a real editor would draw, mixing Ahem's own coverage with characters that force fallback to
// a system face. Surrogates and noncharacters are excluded because shape() requires valid UTF-8.
auto shapeable_text() {
    return fuzztest::VectorOf(fuzztest::ElementOf<char32_t>(
                                  {U'A', U'p', U'É', U' ', U'é', U'Я', U'क',
                                   U'्', U'ก', U'ᄀ', U'\U0001f44d', U'\t', U'\n'}))
        .WithMaxSize(64);
}

// Whatever a backend does with its cluster map, the offsets it reports have to index the string we
// handed it. Getting this wrong is not a crash in fx -- it is a crash in whoever slices the text
// with the result, which on Windows means the hand-rolled inversion in assign_clusters() and on
// the other platforms the index conversion beside it.
void ShapedClustersIndexTheInput(const std::vector<char32_t>& codepoints) {
    std::unique_ptr<fx_font> font = load_ahem();
    ASSERT_TRUE(font);
    std::string utf8 = base::utf32_to_utf8(std::u32string(codepoints.begin(), codepoints.end()));

    std::unique_ptr<fx_layout> layout = font->shape(utf8);
    if (!layout) return;  // a backend may legitimately fail to build a layout

    EXPECT_TRUE(std::isfinite(layout->advance));
    EXPECT_GE(layout->advance, 0.0f);
    EXPECT_TRUE(std::isfinite(layout->line_height));
    for (const fx_glyph& glyph : layout->glyphs) {
        EXPECT_LT(glyph.cluster, utf8.size());
        EXPECT_TRUE(std::isfinite(glyph.x_offset));
        EXPECT_TRUE(std::isfinite(glyph.y_offset));
    }
}
FUZZ_TEST(FxFontFileFuzzTest, ShapedClustersIndexTheInput).WithDomains(shapeable_text());

// The cache keeps one table per platform_value and grows that vector on demand, so a lookup can
// reallocate it while an earlier caller still holds the reference a previous lookup returned.
// Every tile handed out has to survive that, and re-asking for a key has to produce the same tile.
void GlyphCacheKeepsEarlierResultsValid(const std::vector<uint8_t>& platform_values) {
    std::unique_ptr<fx_font> font = load_ahem(FX_FONT_NO_ANTIALIAS);
    ASSERT_TRUE(font);
    std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_FALSE(layout->glyphs.empty());
    uint32_t glyph = layout->glyphs[0].id;

    fx_glyph_cache cache(*font, 1.0f);
    // Sublime indexes the table array by the raw platform value, so keep the fuzzed values inside
    // a range a display could plausibly report rather than letting one allocate 4 billion tables.
    std::vector<std::pair<uint32_t, const fx_glyph_cache::glyph_data*>> seen;
    for (uint8_t value : platform_values) {
        uint32_t platform_value = value % 8u;
        const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(glyph, platform_value);
        seen.emplace_back(platform_value, &data);
    }

    // Every reference taken above must still name the same tile after all the growth.
    for (const auto& [platform_value, first] : seen) {
        const fx_glyph_cache::glyph_data& again = cache.lookup_glyph_data(glyph, platform_value);
        EXPECT_EQ(&again, first) << "table " << platform_value << " moved";
        EXPECT_EQ(again.colored, first->colored);
        EXPECT_EQ(again.phase_at(0).pixels, first->phase_at(0).pixels);
        EXPECT_EQ(again.phase_at(0).width, first->phase_at(0).width);
        EXPECT_EQ(again.phase_at(0).height, first->phase_at(0).height);
    }
}
FUZZ_TEST(FxFontFileFuzzTest, GlyphCacheKeepsEarlierResultsValid);

// ── internal pixel math ─────────────────────────────────────────────────────────────────────────
// Reached through fx_internal.h rather than through fx_font, so each iteration is pixel arithmetic
// instead of a full shape-and-rasterize round trip.

// A scratch buffer the rasterizer could plausibly have produced: a small tile, mostly background,
// with arbitrary pixels scribbled over it.
auto scratch_pixels() {
    return fuzztest::VectorOf(fuzztest::ElementOf<uint32_t>(
                                  {0xff000000u, 0xffffffffu, 0x00000000u, 0xff010203u, 0x80402010u}))
        .WithMinSize(1)
        .WithMaxSize(256);
}

// find_ink promises a tight exclusive bound: nothing outside it differs from the background, and
// every edge touches something that does. The renderers crop tiles with this, so a bound that is
// too small silently clips glyphs and one that is too large wastes atlas space.
void InkBoundsEveryChangedPixel(const std::vector<uint32_t>& pixels, uint8_t raw_width) {
    int width = 1 + raw_width % 16;
    int height = static_cast<int>(pixels.size()) / width;
    if (height < 1) return;

    fx_pixel_buffer buffer{const_cast<uint32_t*>(pixels.data()), width, height, width};
    uint32_t background = pixels[0];
    recti ink = find_ink(buffer, background);

    ASSERT_GE(ink.left, 0);
    ASSERT_GE(ink.top, 0);
    ASSERT_LE(ink.right, width);
    ASSERT_LE(ink.bottom, height);

    bool any_changed = false;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool changed = pixels[static_cast<size_t>(y) * width + x] != background;
            any_changed |= changed;
            bool inside = x >= ink.left && x < ink.right && y >= ink.top && y < ink.bottom;
            if (changed) EXPECT_TRUE(inside) << "changed pixel " << x << "," << y << " outside ink";
        }
    }
    EXPECT_EQ(any_changed, !ink.empty());
}
FUZZ_TEST(FxInternalFuzzTest, InkBoundsEveryChangedPixel)
    .WithDomains(scratch_pixels(), fuzztest::Arbitrary<uint8_t>());

// apply_coverage rewrites the three color channels and must never disturb alpha, whatever the ramp
// says. Inverting twice has to land back where it started, which is what lets the cache store one
// polarity and the renderer read the other.
void CoverageLeavesAlphaAloneAndInvertsBackToItself(const std::vector<uint32_t>& pixels,
                                                    const std::vector<uint8_t>& ramp_values) {
    fx_gamma_ramp ramp;
    for (size_t i = 0; i < ramp.values.size(); ++i) {
        ramp.values[i] = ramp_values.empty() ? static_cast<uint8_t>(i)
                                             : ramp_values[i % ramp_values.size()];
    }

    std::vector<uint32_t> inverted = pixels;
    apply_coverage(inverted, nullptr, true);
    apply_coverage(inverted, nullptr, true);
    EXPECT_EQ(inverted, pixels) << "inverting twice is not the identity";

    std::vector<uint32_t> ramped = pixels;
    apply_coverage(ramped, &ramp, false);
    ASSERT_EQ(ramped.size(), pixels.size());
    for (size_t i = 0; i < pixels.size(); ++i) {
        EXPECT_EQ(ramped[i] >> 24, pixels[i] >> 24) << "alpha changed at " << i;
        for (int shift : {0, 8, 16}) {
            uint8_t before = static_cast<uint8_t>((pixels[i] >> shift) & 0xffu);
            uint8_t after = static_cast<uint8_t>((ramped[i] >> shift) & 0xffu);
            EXPECT_EQ(after, ramp.values[before]) << "channel " << shift << " at " << i;
        }
    }

    std::vector<uint32_t> untouched = pixels;
    apply_coverage(untouched, nullptr, false);
    EXPECT_EQ(untouched, pixels) << "no ramp and no inversion should be a no-op";
}
FUZZ_TEST(FxInternalFuzzTest, CoverageLeavesAlphaAloneAndInvertsBackToItself)
    .WithDomains(scratch_pixels(), fuzztest::VectorOf(fuzztest::Arbitrary<uint8_t>()).WithMaxSize(256));

// Radii a real font can ask for, plus the boundary values around the early return. Deliberately
// bounded: fx_apply_font_glow sizes its kernel straight from the radius, so an arbitrary value
// makes it allocate INT_MAX floats and spend minutes filling them. See the note on the test below.
auto glow_radius() {
    return fuzztest::OneOf(fuzztest::InRange<int>(-4, 64),
                           fuzztest::ElementOf<int>({0, 1, 2, 3}));
}

// The glow reads its background out of pixel zero and blurs a region derived from the ink, so an
// adversarial radius is the interesting input: anything under two pixels must do nothing at all,
// and nothing may write outside the buffer. ASan checks the second part.
//
// The radius domain is bounded on purpose. Left arbitrary, this test found that a radius near
// INT_MAX makes `taps = radius | 1` allocate an 8 GB kernel. fx_rasterize_glyph gets there by
// saturating metrics().ascent * scale, so +inf still reaches it; nothing on that path bounds the
// radius the way the glyph size is bounded.
void GlowIgnoresSmallRadiiAndStaysInBounds(const std::vector<uint32_t>& pixels,
                                           uint8_t raw_width,
                                           int radius) {
    int width = 1 + raw_width % 16;
    int height = static_cast<int>(pixels.size()) / width;
    if (height < 1) return;

    std::vector<uint32_t> scratch(pixels.begin(), pixels.begin() + static_cast<size_t>(width) * height);
    std::vector<uint32_t> before = scratch;
    fx_pixel_buffer buffer{scratch.data(), width, height, width};
    fx_apply_font_glow(&buffer, radius, false);

    if (radius < 2) {
        EXPECT_EQ(scratch, before) << "radius " << radius << " should have been ignored";
    }
}
FUZZ_TEST(FxInternalFuzzTest, GlowIgnoresSmallRadiiAndStaysInBounds)
    .WithDomains(scratch_pixels(), fuzztest::Arbitrary<uint8_t>(), glow_radius());

#if BUILDFLAG(IS_WIN)
// DirectWrite's cluster map is the one piece of shaping arithmetic we invert by hand, so it is the
// one place a malformed run could walk off the end of `glyphs` or of `indices_map`. Feeding it
// cluster maps DirectWrite would never produce is the point: the guards either hold or they do not.
void ClustersOnlyEverNameSuppliedOffsets(const std::vector<uint16_t>& cluster_map,
                                         const std::vector<uint16_t>& utf16,
                                         uint8_t glyph_count,
                                         uint8_t offset_count,
                                         uint32_t text_position) {
    size_t units = std::min(cluster_map.size(), utf16.size());
    if (units == 0) return;

    std::vector<size_t> indices_map(1 + offset_count % 32u);
    for (size_t i = 0; i < indices_map.size(); ++i) indices_map[i] = i * 3u;

    std::vector<fx_glyph> glyphs(1 + glyph_count % 32u);
    for (fx_glyph& glyph : glyphs) glyph.cluster = 0;

    DWRITE_GLYPH_RUN_DESCRIPTION desc{};
    desc.localeName = nullptr;
    desc.string = reinterpret_cast<const WCHAR*>(utf16.data());
    desc.clusterMap = cluster_map.data();
    desc.stringLength = static_cast<UINT32>(units);
    desc.textPosition = text_position;

    assign_clusters(&desc, indices_map, glyphs);

    // Whatever the map said, every cluster has to be a value the caller handed over. Anything else
    // means the inversion read past one of the two arrays.
    for (const fx_glyph& glyph : glyphs) {
        EXPECT_NE(std::find(indices_map.begin(), indices_map.end(), glyph.cluster),
                  indices_map.end())
            << "cluster " << glyph.cluster << " is not an offset we supplied";
    }
}
FUZZ_TEST(FxInternalFuzzTest, ClustersOnlyEverNameSuppliedOffsets);
#endif

TEST(FxFontFileTest, GlowSpreadsAGreyHaloBeyondTheGlyph) {
    // Aliased so the glyph itself is a flat white square; any colour in the tile then has to have
    // come from the blur, which is what guards the monochrome path's single-plane shortcut.
    std::unique_ptr<fx_font> plain = load_ahem(FX_FONT_NO_ANTIALIAS);
    std::unique_ptr<fx_font> glowing = load_ahem(FX_FONT_NO_ANTIALIAS | FX_FONT_GLOW);
    ASSERT_TRUE(plain);
    ASSERT_TRUE(glowing);
    std::unique_ptr<fx_layout> layout = glowing->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);
    uint32_t glyph = layout->glyphs[0].id;

    fx_glyph_cache plain_cache(*plain, 1.0f);
    fx_glyph_cache glow_cache(*glowing, 1.0f);
    const fx_glyph_cache::glyph_phase& before = plain_cache.lookup_glyph_data(glyph).phase_at(0);
    const fx_glyph_cache::glyph_phase& after = glow_cache.lookup_glyph_data(glyph).phase_at(0);
    ASSERT_NE(before.pixels, nullptr);
    ASSERT_NE(after.pixels, nullptr);

    // The halo reaches past the em square on every side.
    EXPECT_GT(after.width, before.width);
    EXPECT_GT(after.height, before.height);

    size_t coloured = 0;
    for (size_t i = 0; i < static_cast<size_t>(after.width) * after.height; ++i) {
        uint32_t pixel = after.pixels[i];
        uint32_t blue = pixel & 0xffu;
        uint32_t green = (pixel >> 8) & 0xffu;
        uint32_t red = (pixel >> 16) & 0xffu;
        if (blue != green || green != red) ++coloured;
    }
    EXPECT_EQ(coloured, 0u) << "a monochrome glyph's glow must stay grey";
}

// ── fonts generate.py builds ────────────────────────────────────────────────────────────────────
// Each isolates one thing the from-file path has to survive, or one shaping property, that Ahem
// cannot express. The generator documents each file's tables.

TEST(FxFontFileTest, RejectsFilesThatAreNotFonts) {
    EXPECT_FALSE(load_test_font("zero_bytes.ttf"));
    EXPECT_FALSE(load_test_font("text.ttf"));
    EXPECT_FALSE(load_test_font("truncated.ttf"));
}

TEST(FxFontFileTest, RejectsSizesThatAreNotPositive) {
    EXPECT_FALSE(load_test_font("Ahem.ttf", 0, 0.0f));
    EXPECT_FALSE(load_test_font("Ahem.ttf", 0, -kSize));
    EXPECT_FALSE(load_test_font("Ahem.ttf", 0, std::numeric_limits<float>::infinity()));
    EXPECT_FALSE(load_test_font("Ahem.ttf", 0, std::numeric_limits<float>::quiet_NaN()));
}

TEST(FxFontFileTest, RejectsZeroUnitsPerEm) {
    // Every design-unit scale divides by unitsPerEm. Core Text and FreeType refuse the file
    // themselves; DirectWrite loads it, and the metrics check has to catch what it produces.
    EXPECT_FALSE(load_test_font("zero_upem.ttf"));
}

TEST(FxFontFileTest, NeverReturnsAFontWithMetricsBelowTheBaseline) {
    // hhea and OS/2 metrics are signed, and this file's ascent is -0.1em with its descent +0.1em.
    // Core Text passes the negative numbers through, giving a one-pixel line height, and
    // DirectWrite cannot represent them and reports zeros; fx refuses both. Pango reports their
    // magnitudes instead, so there the font loads with a 4px line. Either way the editor never
    // sees a metric it cannot size rows with.
    std::unique_ptr<fx_font> font = load_test_font("negative_metrics.ttf");
    if (!font) return;
    fx_font_metrics metrics = font->metrics();
    EXPECT_GE(metrics.ascent, 0.0f);
    EXPECT_GE(metrics.descent, 0.0f);
    EXPECT_GT(metrics.line_height, 0.0f);
    EXPECT_GE(font->raster_ascent(), 0.0f);
}

TEST(FxFontFileTest, ExtremeMetricsLoadAndBoundTheGlow) {
    // Ascent, descent and line gap all at the int16 limit: 32.767em each, 655px at 20px. Which of
    // them a backend folds into the ascent differs, so only the floor is pinned.
    std::unique_ptr<fx_font> font =
        load_test_font("huge_metrics.ttf", FX_FONT_NO_ANTIALIAS | FX_FONT_GLOW);
    ASSERT_TRUE(font);
    fx_font_metrics metrics = font->metrics();
    EXPECT_GE(metrics.ascent, 655.0f);
    EXPECT_GE(metrics.descent, 655.0f);
    EXPECT_GE(metrics.line_height, 1310.0f);
    EXPECT_TRUE(std::isfinite(metrics.line_height));

    // The glow radius is the scaled ascent, 1310px here, which fx_rasterize_glyph caps at 192. The
    // halo reaches half the radius past the em square, so an uncapped one would put a 40px glyph
    // in a tile over 1300px wide.
    fx_glyph_cache cache(*font, 2.0f);
    std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);
    const fx_glyph_cache::glyph_phase& phase =
        cache.lookup_glyph_data(layout->glyphs[0].id).phase_at(0);
    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_GT(phase.width, 40);
    EXPECT_LT(phase.width, 40 + 2 * 192);
    EXPECT_GT(phase.height, 40);
    EXPECT_LT(phase.height, 40 + 2 * 192);
}

TEST(FxFontFileTest, GlyphsTooLargeToRasterizeAreLeftBlank) {
    // 16 units per em is the smallest TrueType allows, and this file's A is 32000 of them square:
    // 2000em, or 40000 device pixels on a side at 20px. The scratch for that is 6.4 GB per phase,
    // so the cache leaves the glyph blank instead of trying.
    std::unique_ptr<fx_font> font = load_test_font("giant.ttf", FX_FONT_NO_ANTIALIAS);
    ASSERT_TRUE(font);
    std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);
    EXPECT_EQ(layout->advance, 40000.0f);

    fx_glyph_cache cache(*font, 1.0f);
    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(layout->glyphs[0].id);
    for (size_t phase = 0; phase < fx_glyph_cache::phase_count; ++phase) {
        EXPECT_EQ(data.phases[phase].pixels, nullptr) << "phase " << phase;
        EXPECT_EQ(data.phases[phase].width, 0) << "phase " << phase;
        EXPECT_EQ(data.phases[phase].height, 0) << "phase " << phase;
    }
}

TEST(FxFontFileTest, LargeGlyphsInsideTheCapAreDrawn) {
    // A 100em square, 2000 pixels on a side at 20px: inside the cap and all ink.
    std::unique_ptr<fx_font> font = load_test_font("large.ttf", FX_FONT_NO_ANTIALIAS);
    ASSERT_TRUE(font);
    std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);
    EXPECT_EQ(layout->advance, 2000.0f);

    fx_glyph_cache cache(*font, 1.0f);
    const fx_glyph_cache::glyph_phase& phase =
        cache.lookup_glyph_data(layout->glyphs[0].id).phase_at(0);
    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_EQ(phase.width, 2000);
    EXPECT_EQ(phase.height, 2000);
    EXPECT_EQ(phase.bearing_x, 0);
    EXPECT_EQ(phase.bearing_y, -2000);
    EXPECT_EQ(phase.pixels[0], 0xffffffffu);
    EXPECT_EQ(phase.pixels[static_cast<size_t>(phase.height) * phase.width - 1], 0xffffffffu);
}

TEST(FxFontFileTest, GlyphsWhoseBearingsOverflowAreLeftBlank) {
    // A 1000-unit square 32000 units left of and below the pen, at 16 units per em: 1250px of ink
    // placed 40000px away. The scratch is small, but no bearing that size fits the cache's 16-bit
    // phase record, and the phase has to stay empty rather than wrap.
    std::unique_ptr<fx_font> font = load_test_font("far.ttf", FX_FONT_NO_ANTIALIAS);
    ASSERT_TRUE(font);
    std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);

    vec2 origin;
    vec2 size;
    font->extents(layout->glyphs[0].id, 1.0f, origin, size);
    EXPECT_GT(origin.x, 32767.0);
    EXPECT_LT(size.x, 1300.0);

    fx_glyph_cache cache(*font, 1.0f);
    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(layout->glyphs[0].id);
    for (size_t phase = 0; phase < fx_glyph_cache::phase_count; ++phase) {
        EXPECT_EQ(data.phases[phase].pixels, nullptr) << "phase " << phase;
    }
}

TEST(FxFontFileTest, FontWithoutCharactersFallsBackForEverything) {
    // Only .notdef and no cmap, so the file's own metrics apply while every character shapes
    // through a system face. The file has Ahem's vertical metrics.
    std::unique_ptr<fx_font> font = load_test_font("notdef_only.ttf");
    ASSERT_TRUE(font);
    fx_font_metrics metrics = font->metrics();
    EXPECT_EQ(metrics.ascent, kAscent);
    EXPECT_EQ(metrics.descent, kDescent);

    std::unique_ptr<fx_layout> layout = font->shape("A");
    ASSERT_TRUE(layout);
    ASSERT_EQ(layout->glyphs.size(), 1u);
    EXPECT_NE(face_of(layout->glyphs[0]), 0u);
    EXPECT_GT(layout->advance, 0.0f);
    EXPECT_GT(font->widths().em_width, 0.0f);
}

TEST(FxFontFileTest, ZeroAdvancesStackGlyphsAtThePen) {
    // Every glyph is an em square that advances nothing, so the layout has to say zero rather than
    // invent a width, while the ink is unaffected.
    std::unique_ptr<fx_font> font = load_test_font("zero_advance.ttf", FX_FONT_NO_ANTIALIAS);
    ASSERT_TRUE(font);
    fx_font_widths widths = font->widths();
    EXPECT_EQ(widths.em_width, 0.0f);
    EXPECT_TRUE(widths.monospace);

    std::unique_ptr<fx_layout> layout = font->shape("AB");
    ASSERT_TRUE(layout);
    EXPECT_EQ(layout->advance, 0.0f);
    ASSERT_EQ(layout->glyphs.size(), 2u);
    EXPECT_EQ(layout->glyphs[0].x_offset, 0.0f);
    EXPECT_EQ(layout->glyphs[1].x_offset, 0.0f);
    EXPECT_EQ(layout->glyphs[0].cluster, 0u);
    EXPECT_EQ(layout->glyphs[1].cluster, 1u);

    fx_glyph_cache cache(*font, 1.0f);
    const fx_glyph_cache::glyph_phase& phase =
        cache.lookup_glyph_data(layout->glyphs[0].id).phase_at(0);
    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_EQ(phase.width, 20);
    EXPECT_EQ(phase.height, 20);
}

struct FeatureCase {
    const char* name;
    const char* text;
    uint32_t attrs;
    size_t glyph_count;
    float advance;
};

TEST(FxFontFileTest, FeatureBitsSelectTheFontsOpenTypeFeatures) {
    // features.ttf gives each feature fx can switch one substitution that changes the advance, so
    // the layout alone shows whether it applied: liga f+i and clig s+t become 1.5em ligatures,
    // dlig c+t likewise, calt narrows an a before b to 0.75em, ss01 halves a, ss10 quarters b, and
    // kern pulls V half an em into A. Everything else is an em box at 20px.
    FeatureCase cases[] = {
        {"liga is on by default", "fi", 0, 1, 30.0f},
        {"NO_LIGA turns liga off", "fi", FX_FONT_NO_LIGA, 2, 40.0f},
        {"clig is on by default", "st", 0, 1, 30.0f},
        {"NO_CLIG turns clig off", "st", FX_FONT_NO_CLIG, 2, 40.0f},
        {"calt is on by default", "ab", 0, 2, 35.0f},
        {"NO_CALT turns calt off", "ab", FX_FONT_NO_CALT, 2, 40.0f},
        {"dlig is off by default", "ct", 0, 2, 40.0f},
        {"DLIG turns dlig on", "ct", FX_FONT_DLIG, 1, 30.0f},
        {"ss01 is off by default", "a", 0, 1, 20.0f},
        {"SS01 turns ss01 on", "a", FX_FONT_SS01, 1, 10.0f},
        {"SS10 turns ss10 on", "b", FX_FONT_SS10, 1, 5.0f},
#if BUILDFLAG(IS_WIN)
        // The typography object Sublime attaches replaces DirectWrite's per-script defaults, and
        // kern is one of those defaults, so Windows renders unkerned as Sublime does.
        {"kerning is off under the typography object", "AV", 0, 2, 40.0f},
#else
        {"kerning applies", "AV", 0, 2, 30.0f},
#endif
    };
    for (const FeatureCase& c : cases) {
        SCOPED_TRACE(c.name);
        std::unique_ptr<fx_font> font = load_test_font("features.ttf", c.attrs);
        ASSERT_TRUE(font);
        std::unique_ptr<fx_layout> layout = font->shape(c.text);
        ASSERT_TRUE(layout);
        EXPECT_EQ(layout->advance, c.advance);
        ASSERT_EQ(layout->glyphs.size(), c.glyph_count);
        for (const fx_glyph& glyph : layout->glyphs) EXPECT_EQ(face_of(glyph), 0u);
        // A ligature carries the cluster of its first character.
        EXPECT_EQ(layout->glyphs[0].cluster, 0u);
        if (c.glyph_count == 2) EXPECT_EQ(layout->glyphs[1].cluster, 1u);
    }
}

}  // namespace
