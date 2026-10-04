#include "build/build_config.h"
#include "fx/fx.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace {

// A font whose rasterizer paints one rectangle wherever the pen it is handed lands, so every
// number the glyph cache derives from a phase has a known answer: the tile is that rectangle
// clipped to the scratch, and its bearings are the rectangle's own offsets from the pen. The real
// backends are only reached through this same interface, so what the cache does with their
// output is checked here at full speed, and the backends themselves against Ahem in
// fx_unittest.cc.
//
// The pen snaps to the pixel at or before it, as a whole-pixel rasterizer would, which makes a
// subpixel phase visible exactly when it carries the pen across a pixel boundary.
class raster_font final : public fx_font {
public:
    uint32_t attrs_value = 0;
    fx_font_metrics metrics_value = {.ascent = 8.0f, .descent = 2.0f, .line_height = 10.0f};
    vec2 extents_origin = {4.0, 12.0};
    vec2 extents_size = {16.0, 16.0};
    // In device pixels relative to the pen, like a bearing, unless `ink_is_absolute` makes them
    // scratch coordinates instead.
    recti ink = {.left = -3, .top = -9, .right = 5, .bottom = 2};
    bool ink_is_absolute = false;
    // Applied to every ink pixel: 1 is solid, less blends toward the foreground the way an
    // antialiased edge would.
    float coverage = 1.0f;
    bool colored = false;
    // What a color glyph paints, premultiplied ARGB, ignoring the foreground.
    uint32_t color_pixel = 0x80ff0000u;
    std::optional<fx_gamma_ramp> ramp;

    // Recorded from the cache's calls.
    int classification_count = 0;
    std::vector<vec2> positions;
    std::vector<float> scales;
    int buffer_width = 0;
    int buffer_height = 0;
    int buffer_row_pixels = 0;
    uint32_t background_word = 0;
    color foreground_seen;
    uint32_t platform_value_seen = 0;

    uint32_t attrs() const override { return attrs_value; }
    fx_font_metrics metrics() const override { return metrics_value; }
    float raster_ascent() const override { return metrics_value.ascent; }
    std::unique_ptr<fx_layout> shape(std::string_view) override { return nullptr; }
    void extents(uint32_t, float, vec2& origin, vec2& size) override {
        origin = extents_origin;
        size = extents_size;
    }
    bool is_color_glyph(uint32_t) override {
        ++classification_count;
        return colored;
    }
    const fx_gamma_ramp* gamma_ramp() const override { return ramp ? &*ramp : nullptr; }

    void rasterize(uint32_t,
                   vec2 position,
                   float scale,
                   fx_pixel_buffer* buffer,
                   color foreground,
                   uint32_t platform_value) override {
        positions.push_back(position);
        scales.push_back(scale);
        buffer_width = buffer->width;
        buffer_height = buffer->height;
        buffer_row_pixels = buffer->row_pixels;
        background_word = buffer->pixels[0];
        foreground_seen = foreground;
        platform_value_seen = platform_value;

        int64_t pen_x = 0;
        int64_t pen_y = 0;
        if (!ink_is_absolute) {
            if (!std::isfinite(position.x) || !std::isfinite(position.y)) return;
            // Wide enough that any pen the cache can produce stays representable.
            constexpr double kReach = 1e15;
            pen_x = static_cast<int64_t>(std::floor(std::clamp(position.x, -kReach, kReach)));
            pen_y = static_cast<int64_t>(std::floor(std::clamp(position.y, -kReach, kReach)));
        }
        int64_t left = std::max<int64_t>(pen_x + ink.left, 0);
        int64_t top = std::max<int64_t>(pen_y + ink.top, 0);
        int64_t right = std::min<int64_t>(pen_x + ink.right, buffer->width);
        int64_t bottom = std::min<int64_t>(pen_y + ink.bottom, buffer->height);
        for (int64_t y = top; y < bottom; ++y) {
            for (int64_t x = left; x < right; ++x) {
                uint32_t& pixel =
                    buffer
                        ->pixels[static_cast<size_t>(y) * static_cast<size_t>(buffer->row_pixels) +
                                 static_cast<size_t>(x)];
                pixel = colored ? color_pixel : blend(pixel, foreground);
            }
        }
    }

private:
    uint32_t blend(uint32_t below, color foreground) const {
        fcolor under = fcolor::from_packed_argb(below);
        fcolor over(foreground);
        auto mix = [this](float a, float b) { return a + (b - a) * coverage; };
        return fcolor{mix(under.r, over.r), mix(under.g, over.g), mix(under.b, over.b),
                      mix(under.a, over.a)}
            .packed_argb();
    }
};

uint32_t channel(uint32_t pixel, int shift) { return (pixel >> shift) & 0xffu; }

constexpr uint32_t kWhite = 0xffffffffu;

// Every phase of a glyph, or the one phase Pango produces.
constexpr size_t kPhases = fx_glyph_cache::phase_count;

TEST(FxGlyphCacheRasterTest, ScratchIsSizedFromTheExtentsWithASpareColumn) {
    raster_font font;
    font.extents_size = {16.2, 15.5};
    fx_glyph_cache cache(font, 1.0f);

    cache.lookup_glyph_data(7, 3);

    // The extra column takes the antialiased edge a phase pushes past the last whole pixel.
    EXPECT_EQ(font.buffer_width, 18);
    EXPECT_EQ(font.buffer_height, 16);
    EXPECT_EQ(font.buffer_row_pixels, 18);
    EXPECT_EQ(font.classification_count, 1);
    ASSERT_EQ(font.positions.size(), kPhases);
    EXPECT_EQ(font.positions[0].x, 4.0);
    EXPECT_EQ(font.positions[0].y, 12.0);
    EXPECT_EQ(font.scales[0], 1.0f);
    EXPECT_EQ(font.platform_value_seen, 3u);
    EXPECT_EQ(font.foreground_seen, color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f));
#if BUILDFLAG(IS_WIN)
    // DirectWrite's read-back leaves untouched pixels as zero words, so the scratch starts that
    // way there rather than as opaque black.
    EXPECT_EQ(font.background_word, 0u);
#else
    EXPECT_EQ(font.background_word, 0xff000000u);
#endif
}

TEST(FxGlyphCacheRasterTest, TileIsTheInkWithBearingsFromThePen) {
    raster_font font;
    font.extents_origin = {4.75, 12.5};
    font.ink = {.left = -3, .top = -9, .right = 5, .bottom = 2};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    EXPECT_FALSE(data.colored);
    const fx_glyph_cache::glyph_phase& phase = data.phase_at(0);
    ASSERT_NE(phase.pixels, nullptr);
    // The pen snapped to (4, 12), the ink went down at (1, 3) to (9, 14), and the bearings are
    // measured from the origin truncated to whole pixels, so they come back as the ink's own
    // offsets.
    EXPECT_EQ(phase.width, 8);
    EXPECT_EQ(phase.height, 11);
    EXPECT_EQ(phase.bearing_x, -3);
    EXPECT_EQ(phase.bearing_y, -9);
    for (size_t i = 0; i < 8u * 11u; ++i) {
        ASSERT_EQ(phase.pixels[i], kWhite) << "pixel " << i;
    }
}

TEST(FxGlyphCacheRasterTest, InkPastTheScratchIsClipped) {
    raster_font font;
    font.extents_origin = {2.0, 2.0};
    font.extents_size = {6.0, 6.0};
    font.ink = {.left = -5, .top = -5, .right = 20, .bottom = 20};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_phase& phase = cache.lookup_glyph_data(7).phase_at(0);

    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_EQ(phase.width, 7);  // the six columns of the extents plus the spare
    EXPECT_EQ(phase.height, 6);
    EXPECT_EQ(phase.bearing_x, -2);
    EXPECT_EQ(phase.bearing_y, -2);
}

TEST(FxGlyphCacheRasterTest, SubpixelPhasesStepThePenAcrossWholePixels) {
    raster_font font;
    font.extents_origin = {4.0, 12.0};
    font.extents_size = {24.0, 16.0};
    // At 6x each sixth-of-a-pixel phase is one whole device pixel, so a whole-pixel rasterizer
    // sees every phase land one column further right. Each tile being exactly eight wide is also
    // what shows the scratch was refilled between phases: otherwise the last one would still
    // hold the columns the earlier phases painted.
    fx_glyph_cache cache(font, 6.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    ASSERT_EQ(font.positions.size(), kPhases);
    for (size_t p = 0; p < kPhases; ++p) {
        SCOPED_TRACE(p);
        EXPECT_NEAR(font.positions[p].x, 4.0 + static_cast<double>(p), 1e-5);
        EXPECT_EQ(font.positions[p].y, 12.0);
        const fx_glyph_cache::glyph_phase& phase = data.phases[p];
        ASSERT_NE(phase.pixels, nullptr);
        EXPECT_EQ(phase.width, 8);
        EXPECT_EQ(phase.height, 11);
        EXPECT_EQ(phase.bearing_x, -3 + static_cast<int>(p));
        EXPECT_EQ(phase.bearing_y, -9);
    }
}

TEST(FxGlyphCacheRasterTest, AlternatePolarityProducesTheSameCoverage) {
#if BUILDFLAG(IS_MAC)
    // An alternate glyph is drawn black on white and XORed back afterwards. For a rasterizer that
    // composites correctly the result is the same coverage the normal polarity produces, here at
    // one fifth so that 51 and 204 both round-trip the XOR exactly.
    raster_font normal;
    normal.coverage = 0.2f;
    raster_font alternate;
    alternate.coverage = 0.2f;
    fx_glyph_cache normal_cache(normal, 1.0f);
    fx_glyph_cache alternate_cache(alternate, 1.0f);

    const fx_glyph_cache::glyph_phase& expected = normal_cache.lookup_glyph_data(7).phase_at(0);
    const fx_glyph_cache::glyph_phase& actual =
        alternate_cache.lookup_glyph_data(7, 0, true).phase_at(0);

    EXPECT_EQ(alternate.background_word, kWhite);
    EXPECT_EQ(alternate.foreground_seen, color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f));
    ASSERT_NE(expected.pixels, nullptr);
    ASSERT_NE(actual.pixels, nullptr);
    ASSERT_EQ(actual.width, expected.width);
    ASSERT_EQ(actual.height, expected.height);
    EXPECT_EQ(actual.bearing_x, expected.bearing_x);
    EXPECT_EQ(actual.bearing_y, expected.bearing_y);
    for (size_t i = 0; i < static_cast<size_t>(expected.width) * expected.height; ++i) {
        ASSERT_EQ(channel(actual.pixels[i], 16), channel(expected.pixels[i], 16)) << i;
        ASSERT_EQ(channel(actual.pixels[i], 8), channel(expected.pixels[i], 8)) << i;
        ASSERT_EQ(channel(actual.pixels[i], 0), channel(expected.pixels[i], 0)) << i;
        ASSERT_EQ(channel(expected.pixels[i], 0), 51u) << i;
    }
#else
    GTEST_SKIP() << "only Core Text smoothing depends on the background";
#endif
}

TEST(FxGlyphCacheRasterTest, GammaRampReshapesMonochromeChannelsAndLeavesAlpha) {
    raster_font font;
    font.coverage = 0.2f;
    font.ramp.emplace();
    for (size_t i = 0; i < font.ramp->values.size(); ++i) {
        font.ramp->values[i] = static_cast<uint8_t>(255 - i);
    }
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_phase& phase = cache.lookup_glyph_data(7).phase_at(0);

    ASSERT_NE(phase.pixels, nullptr);
    uint32_t pixel = phase.pixels[0];
    EXPECT_EQ(channel(pixel, 16), 204u);
    EXPECT_EQ(channel(pixel, 8), 204u);
    EXPECT_EQ(channel(pixel, 0), 204u);
    // Alpha is whatever the rasterizer left, untouched by the ramp: the blend over the platform's
    // background, which is opaque everywhere but on Windows.
#if BUILDFLAG(IS_WIN)
    EXPECT_EQ(channel(pixel, 24), 51u);
#else
    EXPECT_EQ(channel(pixel, 24), 255u);
#endif
}

TEST(FxGlyphCacheRasterTest, ColorGlyphsAreKeptAsPainted) {
    raster_font font;
    font.colored = true;
    font.color_pixel = 0x80ff0000u;
    font.ramp.emplace();
    for (size_t i = 0; i < font.ramp->values.size(); ++i) {
        font.ramp->values[i] = static_cast<uint8_t>(255 - i);
    }
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    EXPECT_TRUE(data.colored);
    // Drawn over transparency on every platform, and neither the ramp nor any inversion touches
    // the result.
    EXPECT_EQ(font.background_word, 0u);
    EXPECT_EQ(font.foreground_seen, color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f));
    const fx_glyph_cache::glyph_phase& phase = data.phase_at(0);
    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_EQ(phase.width, 8);
    EXPECT_EQ(phase.height, 11);
    EXPECT_EQ(phase.pixels[0], 0x80ff0000u);
}

TEST(FxGlyphCacheRasterTest, GlowPadsTheScratchAndKeepsTheHaloNearTheInk) {
    raster_font font;
    font.attrs_value = FX_FONT_GLOW;
    font.metrics_value.ascent = 10.0f;
    font.extents_origin = {4.0, 12.0};
    font.extents_size = {16.0, 16.0};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_phase& phase = cache.lookup_glyph_data(7).phase_at(0);

    // The radius is the scaled ascent, and the scratch grows by twice its ceiling on every side.
    EXPECT_EQ(font.buffer_width, 17 + 40);
    EXPECT_EQ(font.buffer_height, 16 + 40);
    EXPECT_EQ(font.positions[0].x, 24.0);
    EXPECT_EQ(font.positions[0].y, 32.0);
    // The blur reaches half the radius past the ink and no further, and the ink itself stays.
    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_GE(phase.width, 8);
    EXPECT_LE(phase.width, 8 + 10);
    EXPECT_GE(phase.height, 11);
    EXPECT_LE(phase.height, 11 + 10);
    EXPECT_LE(phase.bearing_x, -3);
    EXPECT_GE(phase.bearing_x, -3 - 5);
    EXPECT_LE(phase.bearing_y, -9);
    EXPECT_GE(phase.bearing_y, -9 - 5);
}

TEST(FxGlyphCacheRasterTest, GlowRadiusIsCapped) {
    raster_font font;
    font.attrs_value = FX_FONT_GLOW;
    font.metrics_value.ascent = 1e6f;
    font.extents_size = {16.0, 16.0};
    // Only the padding is under test, and without ink the 193-tap blur has nothing to do, which
    // keeps this off the sanitizer build's critical path.
    font.ink = {};
    fx_glyph_cache cache(font, 2.0f);

    cache.lookup_glyph_data(7);

    // 192 pixels, so the padding is 384 a side however tall the face claims to be.
    EXPECT_EQ(font.buffer_width, 17 + 2 * 384);
    EXPECT_EQ(font.buffer_height, 16 + 2 * 384);
}

struct ExtentsCase {
    const char* name;
    vec2 size;
    bool rasterized;
};

TEST(FxGlyphCacheRasterTest, ExtentsOutsideTheScratchCapAreNotRasterized) {
    constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
    constexpr double kInfinity = std::numeric_limits<double>::infinity();
    // The area cap is 4096 pixels square counting the spare column, so a 4095-wide request is
    // the last one drawn and a 4096-wide one the first left blank. Either side is separately
    // capped at 8192, Skia's limit, whatever the area.
    ExtentsCase cases[] = {
        {"largest area allowed", {4095.0, 4096.0}, true},
        {"one column past the area cap", {4096.0, 4096.0}, false},
        {"widest allowed", {8191.0, 1.0}, true},
        {"one column past the side cap", {8192.0, 1.0}, false},
        {"tallest allowed", {1.0, 8192.0}, true},
        {"one row past the side cap", {1.0, 8193.0}, false},
        {"absurdly wide", {1e9, 1.0}, false},
        {"empty", {0.0, 0.0}, false},
        {"negative", {-1.0, 8.0}, false},
        {"NaN", {kNaN, 8.0}, false},
        {"infinite", {8.0, kInfinity}, false},
    };
    for (const ExtentsCase& c : cases) {
        SCOPED_TRACE(c.name);
        raster_font font;
        font.extents_size = c.size;
        // One pixel of ink at the scratch's own corner, so a scratch one row or one column deep
        // still receives some.
        font.extents_origin = {0.0, 0.0};
        font.ink = {.left = 0, .top = 0, .right = 1, .bottom = 1};
        fx_glyph_cache cache(font, 1.0f);

        const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

        EXPECT_EQ(font.positions.size(), c.rasterized ? kPhases : 0u);
        for (size_t p = 0; p < kPhases; ++p) {
            EXPECT_EQ(data.phases[p].pixels != nullptr, c.rasterized) << "phase " << p;
        }
    }
}

TEST(FxGlyphCacheRasterTest, BearingsThatDoNotFitThePhaseRecordLeaveItEmpty) {
    // The pen sits 40000 pixels from the scratch's origin and the ink lands back inside it, as
    // fx/test_fonts/far.ttf arranges with a real backend. The rasterizer runs, but a 16-bit
    // bearing cannot say where the ink is, so nothing is kept rather than a wrapped offset.
    raster_font font;
    font.extents_origin = {40000.0, 12.0};
    font.ink = {.left = -39998, .top = -9, .right = -39990, .bottom = 2};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    EXPECT_EQ(font.positions.size(), kPhases);
    for (size_t p = 0; p < kPhases; ++p) {
        EXPECT_EQ(data.phases[p].pixels, nullptr) << "phase " << p;
        EXPECT_EQ(data.phases[p].width, 0) << "phase " << p;
    }
}

TEST(FxGlyphCacheRasterTest, SaturatedOriginsDoNotOverflowTheBearing) {
    // An origin beyond int range saturates to INT_MIN, and subtracting it from the ink's position
    // has to widen first or it overflows. The phase is empty either way; UBSan is what tells the
    // two apart.
    raster_font font;
    font.extents_origin = {-1e300, -1e300};
    font.ink_is_absolute = true;
    font.ink = {.left = 2, .top = 3, .right = 6, .bottom = 8};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    EXPECT_EQ(font.positions.size(), kPhases);
    EXPECT_EQ(data.phase_at(0).pixels, nullptr);
}

TEST(FxGlyphCacheRasterTest, PensThatAreNotFiniteDrawNothing) {
    raster_font font;
    font.extents_origin = {std::numeric_limits<double>::quiet_NaN(), 12.0};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    EXPECT_EQ(font.positions.size(), kPhases);
    EXPECT_EQ(data.phase_at(0).pixels, nullptr);
}

TEST(FxGlyphCacheRasterTest, NoInkLeavesEveryPhaseEmpty) {
    raster_font font;
    font.ink = {};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_data& data = cache.lookup_glyph_data(7);

    EXPECT_EQ(font.positions.size(), kPhases);
    for (size_t p = 0; p < kPhases; ++p) {
        EXPECT_EQ(data.phases[p].pixels, nullptr) << "phase " << p;
        EXPECT_EQ(data.phases[p].width, 0) << "phase " << p;
        EXPECT_EQ(data.phases[p].height, 0) << "phase " << p;
    }
}

// Everything the unit tests above pin, over arbitrary placements: wherever the pen and the ink
// land, the tile is the ink clipped to the scratch and the bearings say where it went.
void TileIsTheClippedInk(double origin_x,
                         double origin_y,
                         double width,
                         double height,
                         int left,
                         int top,
                         int ink_width,
                         int ink_height) {
    raster_font font;
    font.extents_origin = {origin_x, origin_y};
    font.extents_size = {width, height};
    font.ink = {.left = left, .top = top, .right = left + ink_width, .bottom = top + ink_height};
    fx_glyph_cache cache(font, 1.0f);

    const fx_glyph_cache::glyph_phase& phase = cache.lookup_glyph_data(7).phase_at(0);

    int pen_x = static_cast<int>(std::floor(origin_x));
    int pen_y = static_cast<int>(std::floor(origin_y));
    int scratch_width = static_cast<int>(std::ceil(width)) + 1;
    int scratch_height = static_cast<int>(std::ceil(height));
    int ink_left = std::max(pen_x + left, 0);
    int ink_top = std::max(pen_y + top, 0);
    int ink_right = std::min(pen_x + left + ink_width, scratch_width);
    int ink_bottom = std::min(pen_y + top + ink_height, scratch_height);
    if (ink_right <= ink_left || ink_bottom <= ink_top) {
        EXPECT_EQ(phase.pixels, nullptr);
        return;
    }
    ASSERT_NE(phase.pixels, nullptr);
    EXPECT_EQ(phase.width, ink_right - ink_left);
    EXPECT_EQ(phase.height, ink_bottom - ink_top);
    EXPECT_EQ(phase.bearing_x, ink_left - pen_x);
    EXPECT_EQ(phase.bearing_y, ink_top - pen_y);
    for (size_t i = 0; i < static_cast<size_t>(phase.width) * phase.height; ++i) {
        ASSERT_EQ(phase.pixels[i], kWhite) << "pixel " << i;
    }
}
FUZZ_TEST(FxGlyphCacheRasterFuzzTest, TileIsTheClippedInk)
    .WithDomains(fuzztest::InRange(0.0, 64.0),
                 fuzztest::InRange(0.0, 64.0),
                 fuzztest::InRange(1.0, 64.0),
                 fuzztest::InRange(1.0, 64.0),
                 fuzztest::InRange(-80, 80),
                 fuzztest::InRange(-80, 80),
                 fuzztest::InRange(0, 80),
                 fuzztest::InRange(0, 80));

}  // namespace
