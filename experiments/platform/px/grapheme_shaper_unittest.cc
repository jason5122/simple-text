#include "experiments/platform/px/grapheme_shaper.h"

#include "base/unicode/unicode.h"
#include "experiments/platform/px/px_font_internal.h"
#include "experiments/platform/ui/retained_text.h"

#include <fuzztest/fuzztest_core.h>
#include <fuzztest/init_fuzztest.h>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::u32string decode_utf8(std::string_view input) {
    std::u32string result;
    for (size_t offset = 0; offset < input.size();) {
        const base::Unichar codepoint = base::next_utf8(input, offset);
        result.push_back(codepoint < 0 ? U'\ufffd' : static_cast<char32_t>(codepoint));
    }
    return result;
}

size_t utf8_length(char32_t codepoint) {
    const int length = base::codepoint_to_utf8(static_cast<base::Unichar>(codepoint));
    return length < 0 ? 3 : static_cast<size_t>(length);
}

std::string encode_utf8(std::u32string_view input) {
    std::string result;
    for (char32_t codepoint : input) {
        char bytes[4];
        const int length = base::codepoint_to_utf8(static_cast<base::Unichar>(codepoint), bytes);
        if (length > 0) {
            result.append(bytes, static_cast<size_t>(length));
        }
    }
    return result;
}

class fake_font final : public fx_font {
public:
    explicit fake_font(bool monospace) : monospace_(monospace) {}

    uint32_t attrs() const override { return 0; }
    fx_font_metrics metrics() const override {
        return {.ascent = 8.0f, .descent = 2.0f, .line_height = 10.0f};
    }
    float raster_ascent() const override { return 8.0f; }

    std::unique_ptr<fx_layout> shape(std::string_view utf8) override {
        return shape_text(decode_utf8(utf8));
    }

    std::unique_ptr<fx_layout> shape(std::u32string_view utf32) override {
        return shape_text(std::u32string(utf32));
    }

    void extents(uint32_t, float, vec2&, vec2&) override {}
    fx_glyph_bitmap rasterise(uint32_t, vec2, float) override { return {}; }
    bool is_color_glyph(uint32_t) override { return false; }
    bool bg_affects_rasterise() const override { return false; }
    const fx_gamma_ramp* gamma_ramp() const override { return nullptr; }

    void clear_shaped_texts() { shaped_texts_.clear(); }
    const std::vector<std::u32string>& get_shaped_texts() const { return shaped_texts_; }
    void set_large_cluster_glyph_count(size_t value) { large_cluster_glyph_count_ = value; }
    void set_glyph_advance(float value) { glyph_advance_ = value; }

private:
    std::unique_ptr<fx_layout> shape_text(std::u32string text) {
        shaped_texts_.push_back(text);

        auto layout = std::make_unique<fx_layout>();
        layout->line_height = 10.0f;
        if (text == U"a\u0301" && large_cluster_glyph_count_ != 0) {
            layout->advance = static_cast<float>(large_cluster_glyph_count_);
            for (size_t index = 0; index < large_cluster_glyph_count_; ++index) {
                layout->glyphs.push_back({.id = static_cast<uint32_t>(1000 + index)});
            }
            return layout;
        }
        if (text == U"->") {
            layout->advance = 2.0f;
            layout->glyphs.push_back({.id = 0xfade});
            return layout;
        }

        uint32_t byte_offset = 0;
        for (char32_t codepoint : text) {
            const float advance = !monospace_ && codepoint == U'M' ? 2.0f : glyph_advance_;
            layout->glyphs.push_back({
                .id = static_cast<uint32_t>(codepoint),
                .x_offset = layout->advance,
                .cluster = byte_offset,
            });
            layout->advance += advance;
            byte_offset += static_cast<uint32_t>(utf8_length(codepoint));
        }
        return layout;
    }

    bool monospace_ = true;
    float glyph_advance_ = 1.0f;
    size_t large_cluster_glyph_count_ = 0;
    std::vector<std::u32string> shaped_texts_;
};

struct shaper_fixture {
    explicit shaper_fixture(bool monospace = true, uint32_t flags = 0)
        : native_font(std::make_unique<fake_font>(monospace)),
          fake(native_font.get()),
          font("fake", 10.0f, 0, std::move(native_font)),
          shaper(&font, flags) {
        fake->clear_shaped_texts();
    }

    std::unique_ptr<fake_font> native_font;
    fake_font* fake;
    px_font_t font;
    grapheme_shaper shaper;
};

struct recorded_draw {
    enum class mode {
        normal,
        faded,
        clipped,
    };

    vec2 position;
    ::color color;
    fx_layout layout;
    bool subpixel_positioning = false;
    mode render_mode = mode::normal;
    float range_start = 0.0f;
    float range_end = 0.0f;
};

class recording_context final : public px_render_context {
public:
    void draw_rect(rect, fill_mode) override {}
    void draw_shaped_text(px_font_t*,
                          vec2 position,
                          color value,
                          fx_layout* layout,
                          bool subpixel_positioning) override {
        draws.push_back({position, value, *layout, subpixel_positioning});
    }
    void draw_shaped_text_faded(px_font_t*,
                                vec2 position,
                                color value,
                                fx_layout* layout,
                                bool subpixel_positioning,
                                float fade_start,
                                float fade_end) override {
        draws.push_back({position, value, *layout, subpixel_positioning,
                         recorded_draw::mode::faded, fade_start, fade_end});
    }
    void draw_shaped_text_clipped(px_font_t*,
                                  vec2 position,
                                  color value,
                                  fx_layout* layout,
                                  bool subpixel_positioning,
                                  float clip_start,
                                  float clip_end) override {
        draws.push_back({position, value, *layout, subpixel_positioning,
                         recorded_draw::mode::clipped, clip_start, clip_end});
    }
    void translate(double, double) override {}
    void scale(double, double) override {}
    void restrict_clip_rect(rect) override {}
    void push_state(bool) override {}
    void pop_state() override {}
    vec2 get_translation() override { return {}; }
    vec2 get_scale() override { return {1.0, 1.0}; }
    recti get_clip_rect() override { return {}; }
    double dpi_scale_factor() override { return 1.0; }

    std::vector<recorded_draw> draws;
};

void convert_utf8_clusters_to_codepoint_indices(recording_context* context,
                                                std::string_view utf8) {
    std::vector<uint32_t> codepoint_indices(utf8.size() + 1, UINT32_MAX);
    size_t byte_offset = 0;
    uint32_t codepoint_index = 0;
    codepoint_indices[0] = 0;
    while (byte_offset < utf8.size()) {
        base::next_utf8(utf8, byte_offset);
        codepoint_indices[byte_offset] = ++codepoint_index;
    }

    for (recorded_draw& draw : context->draws) {
        for (fx_glyph& glyph : draw.layout.glyphs) {
            ASSERT_LT(glyph.cluster, codepoint_indices.size());
            ASSERT_NE(codepoint_indices[glyph.cluster], UINT32_MAX);
            glyph.cluster = codepoint_indices[glyph.cluster];
        }
    }
}

void expect_equivalent_draws(const std::vector<recorded_draw>& left,
                             const std::vector<recorded_draw>& right) {
    ASSERT_EQ(left.size(), right.size());
    for (size_t draw_index = 0; draw_index < left.size(); ++draw_index) {
        SCOPED_TRACE(testing::Message() << "draw " << draw_index);
        const recorded_draw& left_draw = left[draw_index];
        const recorded_draw& right_draw = right[draw_index];
        EXPECT_DOUBLE_EQ(left_draw.position.x, right_draw.position.x);
        EXPECT_DOUBLE_EQ(left_draw.position.y, right_draw.position.y);
        EXPECT_EQ(left_draw.color, right_draw.color);
        EXPECT_FLOAT_EQ(left_draw.layout.advance, right_draw.layout.advance);
        EXPECT_FLOAT_EQ(left_draw.layout.line_height, right_draw.layout.line_height);
        EXPECT_EQ(left_draw.subpixel_positioning, right_draw.subpixel_positioning);
        EXPECT_EQ(left_draw.render_mode, right_draw.render_mode);
        ASSERT_EQ(left_draw.layout.glyphs.size(), right_draw.layout.glyphs.size());

        for (size_t glyph_index = 0; glyph_index < left_draw.layout.glyphs.size(); ++glyph_index) {
            SCOPED_TRACE(testing::Message() << "glyph " << glyph_index);
            const fx_glyph& left_glyph = left_draw.layout.glyphs[glyph_index];
            const fx_glyph& right_glyph = right_draw.layout.glyphs[glyph_index];
            EXPECT_EQ(left_glyph.id, right_glyph.id);
            EXPECT_FLOAT_EQ(left_glyph.x_offset, right_glyph.x_offset);
            EXPECT_FLOAT_EQ(left_glyph.y_offset, right_glyph.y_offset);
            EXPECT_EQ(left_glyph.cluster, right_glyph.cluster);
        }
    }
}

double drawn_advance(const recording_context& context, double origin_x) {
    if (context.draws.empty()) {
        return 0.0;
    }
    const recorded_draw& last = context.draws.back();
    return last.position.x - origin_x + last.layout.advance;
}

auto shaper_text() {
    return fuzztest::VectorOf(
               fuzztest::ElementOf<char32_t>(
                   {U'a',          U'b',          U'M',          U' ',          U'\t',
                    U'\n',         U'-',          U'>',          U'=',          U'\u0001',
                    U'\u007f',     U'\u0080',     U'\u0301',     U'\u061c',     U'\u0915',
                    U'\u0937',     U'\u094d',     U'\u0e01',     U'\u0e33',     U'\u0e81',
                    U'\u0eb3',     U'\u1100',     U'\u1161',     U'\u11a8',     U'\u200d',
                    U'\u2602',     U'\ufe0f',     U'\U0001f3fd', U'\U0001f44d', U'\U0001f469',
                    U'\U0001f4bb', U'\U0001f1e6', U'\U0001f1e7', U'\U0001f1e8'}))
        .WithMaxSize(128);
}

auto ordinary_shaper_text() {
    return fuzztest::VectorOf(
               fuzztest::ElementOf<char32_t>(
                   {U'a',          U'b',          U'M',          U' ',          U'-',
                    U'>',          U'=',          U'\u0301',     U'\u0915',     U'\u0937',
                    U'\u094d',     U'\u0e01',     U'\u0e81',     U'\u1100',     U'\u1161',
                    U'\u11a8',     U'\u2602',     U'\ufe0f',     U'\U0001f3fd', U'\U0001f44d',
                    U'\U0001f469', U'\U0001f4bb', U'\U0001f1e6', U'\U0001f1e7', U'\U0001f1e8'}))
        .WithMaxSize(128);
}

TEST(GraphemeShaperTest, EmptyStringsProduceNoBatches) {
    shaper_fixture fixture;

    EXPECT_TRUE(prepare_retained_text(&fixture.shaper, std::string_view{}).batches.empty());
    EXPECT_TRUE(prepare_retained_text(&fixture.shaper, std::u32string_view{}).batches.empty());
    EXPECT_TRUE(fixture.fake->get_shaped_texts().empty());
}

TEST(GraphemeShaperTest, SublimeBoundariesKeepHangulAndIndicCodepointsSeparate) {
    shaper_fixture fixture;

    prepare_retained_text(&fixture.shaper, U"\u1100\u1161\u11a8X");
    EXPECT_EQ(fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"\u1100", U"\u1161", U"\u11a8", U"X"}));

    fixture.fake->clear_shaped_texts();
    prepare_retained_text(&fixture.shaper, U"\u0915\u094d\u0937Y");
    EXPECT_EQ(fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"\u0915\u094d", U"\u0937", U"Y"}));
}

TEST(GraphemeShaperTest, ThaiAndLaoSaraAmExtendThePrecedingCluster) {
    const std::vector<std::u32string> expected = {U"\u0e01\u0e33", U"X", U"\u0e81\u0eb3", U"Y"};
    shaper_fixture utf32_fixture;

    prepare_retained_text(&utf32_fixture.shaper, U"\u0e01\u0e33X\u0e81\u0eb3Y");
    EXPECT_EQ(utf32_fixture.fake->get_shaped_texts(), expected);

    shaper_fixture utf8_fixture;
    prepare_retained_text(&utf8_fixture.shaper,
                          "\xe0\xb8\x81\xe0\xb8\xb3X\xe0\xba\x81\xe0\xba\xb3Y");
    EXPECT_EQ(utf8_fixture.fake->get_shaped_texts(), expected);
}

TEST(GraphemeShaperTest, Utf8AndUtf32UseTheSameCombiningAndEmojiBoundaries) {
    const std::u32string text = U"a\u0301\U0001f469\u200d\U0001f4bbX";
    shaper_fixture utf32_fixture;
    prepare_retained_text(&utf32_fixture.shaper, text);

    ASSERT_EQ(utf32_fixture.fake->get_shaped_texts().size(), 3u);
    EXPECT_EQ(utf32_fixture.fake->get_shaped_texts()[0], U"a\u0301");
    EXPECT_EQ(utf32_fixture.fake->get_shaped_texts()[1], U"\U0001f469\u200d\U0001f4bb");
    EXPECT_EQ(utf32_fixture.fake->get_shaped_texts()[2], U"X");

    shaper_fixture utf8_fixture;
    prepare_retained_text(&utf8_fixture.shaper,
                          "a\xcc\x81\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x92\xbbX");
    EXPECT_EQ(utf8_fixture.fake->get_shaped_texts(), utf32_fixture.fake->get_shaped_texts());
}

TEST(GraphemeShaperTest, VariationSelectorsSkinTonesAndRegionalIndicatorsStayGrouped) {
    shaper_fixture fixture;

    prepare_retained_text(&fixture.shaper,
                          U"\u2602\ufe0f\U0001f44d\U0001f3fd\U0001f1e6\U0001f1e7\U0001f1e8X");

    EXPECT_EQ(fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"\u2602\ufe0f", U"\U0001f44d\U0001f3fd",
                                           U"\U0001f1e6\U0001f1e7", U"\U0001f1e8", U"X"}));
}

TEST(GraphemeShaperTest, MonospaceOperatorRunsCanFormLigatures) {
    shaper_fixture monospace_fixture;
    const retained_text monospace = prepare_retained_text(&monospace_fixture.shaper, "a->b");

    EXPECT_EQ(monospace_fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"a", U"->", U"b"}));
    ASSERT_EQ(monospace.batches.size(), 1u);
    ASSERT_EQ(monospace.batches[0].layout.glyphs.size(), 3u);
    EXPECT_EQ(monospace.batches[0].layout.glyphs[1].id, 0xfadeu);
    EXPECT_DOUBLE_EQ(monospace.advance, 4.0);

    shaper_fixture proportional_fixture(false);
    prepare_retained_text(&proportional_fixture.shaper, "a->b");
    EXPECT_EQ(proportional_fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"a", U"-", U">", U"b"}));
}

TEST(GraphemeShaperTest, BatchThresholdKeepsThirtyTwoGlyphsTogether) {
    shaper_fixture fixture;

    const retained_text prepared_31 = prepare_retained_text(&fixture.shaper, std::string(31, 'a'));
    const retained_text prepared_32 = prepare_retained_text(&fixture.shaper, std::string(32, 'b'));
    const retained_text prepared_33 = prepare_retained_text(&fixture.shaper, std::string(33, 'c'));
    const auto& batches_31 = prepared_31.batches;
    const auto& batches_32 = prepared_32.batches;
    const auto& batches_33 = prepared_33.batches;

    ASSERT_EQ(batches_31.size(), 1u);
    EXPECT_EQ(batches_31[0].layout.glyphs.size(), 31u);
    ASSERT_EQ(batches_32.size(), 1u);
    EXPECT_EQ(batches_32[0].layout.glyphs.size(), 32u);
    ASSERT_EQ(batches_33.size(), 2u);
    EXPECT_EQ(batches_33[0].layout.glyphs.size(), 32u);
    EXPECT_EQ(batches_33[1].layout.glyphs.size(), 1u);
    EXPECT_DOUBLE_EQ(batches_33[1].x_offset, 32.0);
}

TEST(GraphemeShaperTest, LargeSingleClusterIsEmittedWithoutSplitting) {
    shaper_fixture fixture;
    fixture.fake->set_large_cluster_glyph_count(33);

    const retained_text prepared = prepare_retained_text(&fixture.shaper, U"a\u0301");
    const auto& batches = prepared.batches;

    ASSERT_EQ(batches.size(), 1u);
    EXPECT_EQ(batches[0].layout.glyphs.size(), 33u);
    EXPECT_FLOAT_EQ(batches[0].layout.advance, 33.0f);
}

TEST(GraphemeShaperTest, ControlLabelsUseHexOrMnemonicPolicy) {
    shaper_fixture hexadecimal_fixture;
    prepare_retained_text(&hexadecimal_fixture.shaper, U"\u0001");
    ASSERT_EQ(hexadecimal_fixture.fake->get_shaped_texts().size(), 1u);
    EXPECT_EQ(hexadecimal_fixture.fake->get_shaped_texts()[0], U"<0x01>");

    shaper_fixture mnemonic_fixture(true, GRAPHEME_SHAPER_USE_CONTROL_NAMES);
    prepare_retained_text(&mnemonic_fixture.shaper, U"\u0001");
    ASSERT_EQ(mnemonic_fixture.fake->get_shaped_texts().size(), 1u);
    EXPECT_EQ(mnemonic_fixture.fake->get_shaped_texts()[0], U"<SOH>");
}

TEST(GraphemeShaperTest, ControlFlagsSelectOnlyTheirOwnCharacterClasses) {
    shaper_fixture default_fixture;
    prepare_retained_text(&default_fixture.shaper, U"\u0001\u007f\u0080\t\n\u00a0\u061c\u115f");
    EXPECT_EQ(default_fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"<0x01>", U"<0x7f>", U"<0x80>", U"\t", U"\n",
                                           U"\u00a0", U"\u061c", U"\u115f"}));

    shaper_fixture named_fixture(
        true, GRAPHEME_SHAPER_DRAW_COMMON_WHITESPACE | GRAPHEME_SHAPER_DRAW_BIDI |
                  GRAPHEME_SHAPER_DRAW_ALL_WHITESPACE | GRAPHEME_SHAPER_USE_CONTROL_NAMES);
    prepare_retained_text(&named_fixture.shaper, U"\u00a0\u061c\u115f");
    EXPECT_EQ(named_fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"<NBSP>", U"<ALM>", U"<HCFIL>"}));
}

TEST(GraphemeShaperTest, Utf32GlyphClustersUseCodepointIndices) {
    shaper_fixture fixture;

    const retained_text prepared = prepare_retained_text(&fixture.shaper, U"a\u0301\U0001f600");
    const auto& batches = prepared.batches;

    ASSERT_EQ(batches.size(), 1u);
    ASSERT_EQ(batches[0].layout.glyphs.size(), 3u);
    EXPECT_EQ(batches[0].layout.glyphs[0].cluster, 0u);
    EXPECT_EQ(batches[0].layout.glyphs[1].cluster, 1u);
    EXPECT_EQ(batches[0].layout.glyphs[2].cluster, 2u);
}

TEST(GraphemeShaperTest, Utf8GlyphClustersUseByteOffsets) {
    shaper_fixture fixture;

    const retained_text prepared =
        prepare_retained_text(&fixture.shaper, "a\xcc\x81\xf0\x9f\x98\x80");
    const auto& batches = prepared.batches;

    ASSERT_EQ(batches.size(), 1u);
    ASSERT_EQ(batches[0].layout.glyphs.size(), 3u);
    EXPECT_EQ(batches[0].layout.glyphs[0].cluster, 0u);
    EXPECT_EQ(batches[0].layout.glyphs[1].cluster, 1u);
    EXPECT_EQ(batches[0].layout.glyphs[2].cluster, 3u);
}

TEST(RetainedTextTest, NativeFontOverloadShapesTheWholeStringOnce) {
    shaper_fixture fixture(false);

    const retained_text prepared = prepare_retained_text(&fixture.font, "AV->");

    EXPECT_EQ(fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"AV->"}));
    ASSERT_EQ(prepared.batches.size(), 1u);
    EXPECT_DOUBLE_EQ(prepared.advance, 4.0);
}

TEST(RetainedTextTest, ReplaysCapturedLayoutsWithTheRequestedOrigin) {
    shaper_fixture fixture;
    retained_text prepared = prepare_retained_text(&fixture.shaper, std::string(33, 'a'));
    recording_context context;
    const fcolor color{0.1f, 0.2f, 0.3f, 0.4f};

    draw_retained_text(&context, &fixture.font, {10.0, 20.0}, color, &prepared);

    ASSERT_EQ(context.draws.size(), 2u);
    EXPECT_DOUBLE_EQ(context.draws[0].position.x, 10.0);
    EXPECT_DOUBLE_EQ(context.draws[0].position.y, 20.0);
    EXPECT_TRUE(context.draws[0].subpixel_positioning);
    EXPECT_DOUBLE_EQ(context.draws[1].position.x, 42.0);
}

TEST(GraphemeShaperTest, ImmediateDrawingUsesSharedBatchPositionsWithoutRetainingLayouts) {
    shaper_fixture fixture;
    recording_context context;
    const fcolor color{0.1f, 0.2f, 0.3f, 0.4f};

    fixture.shaper.draw_string(&context, 10.0, 20.0, color, std::string(33, 'a'), false, {}, false,
                               0.0f, 0.0f);

    ASSERT_EQ(context.draws.size(), 2u);
    EXPECT_DOUBLE_EQ(context.draws[0].position.x, 10.0);
    EXPECT_DOUBLE_EQ(context.draws[0].position.y, 20.0);
    EXPECT_EQ(context.draws[0].layout.glyphs.size(), 32u);
    EXPECT_FALSE(context.draws[0].subpixel_positioning);
    EXPECT_EQ(context.draws[0].color, static_cast<::color>(color));
    EXPECT_DOUBLE_EQ(context.draws[1].position.x, 42.0);
    EXPECT_EQ(context.draws[1].layout.glyphs.size(), 1u);
}

TEST(GraphemeShaperTest, Utf32SpaceMarkersUseMiddleDotAndTheirOwnColor) {
    shaper_fixture fixture;
    recording_context context;
    const fcolor ordinary_color{0.1f, 0.2f, 0.3f, 0.4f};
    const fcolor space_color{0.5f, 0.6f, 0.7f, 0.8f};
    fixture.shaper.draw_string(&context, 10.0, 20.0, ordinary_color, U"a b", true, space_color,
                               false, 0.0f, 0.0f);

    ASSERT_EQ(context.draws.size(), 3u);
    EXPECT_DOUBLE_EQ(context.draws[0].position.x, 10.0);
    EXPECT_EQ(context.draws[0].layout.glyphs[0].id, static_cast<uint32_t>(U'a'));
    EXPECT_EQ(context.draws[0].color, static_cast<::color>(ordinary_color));
    EXPECT_DOUBLE_EQ(context.draws[1].position.x, 11.0);
    EXPECT_EQ(context.draws[1].layout.glyphs[0].id, static_cast<uint32_t>(U'\u00b7'));
    EXPECT_EQ(context.draws[1].color, static_cast<::color>(space_color));
    EXPECT_DOUBLE_EQ(context.draws[2].position.x, 12.0);
    EXPECT_EQ(context.draws[2].layout.glyphs[0].id, static_cast<uint32_t>(U'b'));
    EXPECT_EQ(context.draws[2].color, static_cast<::color>(ordinary_color));
    EXPECT_EQ(fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"a", U"\u00b7", U"b"}));
}

TEST(GraphemeShaperTest, DrawOptionsDispatchOverloadSpecificRenderModes) {
    shaper_fixture fixture;
    recording_context context;
    const fcolor color{0.1f, 0.2f, 0.3f, 0.4f};

    fixture.shaper.draw_string(&context, 1.0, 2.0, color, "ab", false, {}, true, 3.0f, 4.0f);
    fixture.shaper.draw_string(&context, 5.0, 6.0, color, U"cd", false, {}, true, 7.0f, 8.0f);

    ASSERT_EQ(context.draws.size(), 2u);
    EXPECT_EQ(context.draws[0].render_mode, recorded_draw::mode::faded);
    EXPECT_FALSE(context.draws[0].subpixel_positioning);
    EXPECT_FLOAT_EQ(context.draws[0].range_start, 3.0f);
    EXPECT_FLOAT_EQ(context.draws[0].range_end, 4.0f);
    EXPECT_EQ(context.draws[1].render_mode, recorded_draw::mode::clipped);
    EXPECT_FALSE(context.draws[1].subpixel_positioning);
    EXPECT_FLOAT_EQ(context.draws[1].range_start, 7.0f);
    EXPECT_FLOAT_EQ(context.draws[1].range_end, 8.0f);
}

TEST(GraphemeShaperTest, MeasurementUsesGraphemesRatherThanDrawOperatorRuns) {
    shaper_fixture fixture;

    EXPECT_FLOAT_EQ(fixture.shaper.measure_string(U"a->b"), 4.0f);
    EXPECT_FLOAT_EQ(fixture.shaper.measure_string(U"a\nb"), 3.0f);
    EXPECT_EQ(fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U"a", U"-", U">", U"b", U" "}));
}

TEST(GraphemeShaperTest, MeasurementMapsStandaloneThaiAndLaoSaraAmToSaraAa) {
    shaper_fixture utf32_fixture;
    EXPECT_FLOAT_EQ(utf32_fixture.shaper.measure_string(U"\u0e33"), 1.0f);
    EXPECT_EQ(utf32_fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"\u0e32"}));
    utf32_fixture.fake->clear_shaped_texts();
    EXPECT_FLOAT_EQ(utf32_fixture.shaper.measure_string(U"\u0eb3"), 1.0f);
    EXPECT_EQ(utf32_fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"\u0eb2"}));

    shaper_fixture utf8_fixture;
    EXPECT_FLOAT_EQ(utf8_fixture.shaper.measure_string("\xe0\xb8\xb3"), 1.0f);
    EXPECT_EQ(utf8_fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"\u0e32"}));
    utf8_fixture.fake->clear_shaped_texts();
    EXPECT_FLOAT_EQ(utf8_fixture.shaper.measure_string("\xe0\xba\xb3"), 1.0f);
    EXPECT_EQ(utf8_fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"\u0eb2"}));
}

TEST(GraphemeShaperTest, MeasurementsAndDrawsReuseCachedGraphemeLayouts) {
    shaper_fixture fixture;

    EXPECT_FLOAT_EQ(fixture.shaper.measure_string(U"aba"), 3.0f);
    EXPECT_EQ(fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"a", U"b"}));

    recording_context context;
    fixture.shaper.draw_string(&context, 0.0, 0.0, {}, U"aba", false, {}, false, 0.0f, 0.0f);

    EXPECT_EQ(fixture.fake->get_shaped_texts(), (std::vector<std::u32string>{U"a", U"b"}));
    ASSERT_EQ(context.draws.size(), 1u);
    EXPECT_FLOAT_EQ(context.draws[0].layout.advance, 3.0f);
}

TEST(GraphemeShaperTest, OperatorRunsPreemptFollowingGraphemeGroupingDuringDrawing) {
    constexpr std::u32string_view text = U">-\u200d\u007f";
    shaper_fixture measurement_fixture;

    EXPECT_FLOAT_EQ(measurement_fixture.shaper.measure_string(text), 4.0f);
    EXPECT_EQ(measurement_fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U">", U"-\u200d\u007f"}));

    shaper_fixture drawing_fixture;
    recording_context context;
    drawing_fixture.shaper.draw_string(&context, 0.0, 0.0, {}, text, false, {}, false, 0.0f, 0.0f);

    EXPECT_DOUBLE_EQ(drawn_advance(context, 0.0), 9.0);
    EXPECT_EQ(drawing_fixture.fake->get_shaped_texts(),
              (std::vector<std::u32string>{U">-", U"\u200d", U"<0x7f>"}));
}

#if defined(__APPLE__) || defined(_WIN32)
TEST(SystemFontIntegrationTest, SystemAliasProvidesUsableMetricsAndShapesText) {
    px_font_t* font = px_create_font("system", 12.0f);
    ASSERT_NE(font, nullptr);

    const px_font_metrics metrics = px_font_get_metrics(font);
    EXPECT_GT(metrics.ascent, 0.0f);
    EXPECT_GE(metrics.descent, 0.0f);
    EXPECT_GT(metrics.line_height, 0.0f);
    EXPECT_GT(metrics.raster_ascent, 0.0f);
    EXPECT_GT(px_font_em_width(font), 0.0f);

    recording_context context;
    context.draw_text(font, {0.0, 0.0}, {}, "Aa", true);
    ASSERT_EQ(context.draws.size(), 1u);
    EXPECT_FALSE(context.draws[0].layout.glyphs.empty());
    EXPECT_GT(context.draws[0].layout.advance, 0.0f);
}

TEST(SystemFontIntegrationTest, GlyphExtentsAreFiniteAndNonempty) {
    px_font_t* font = px_create_font("system", 12.0f);
    ASSERT_NE(font, nullptr);

    const std::unique_ptr<fx_layout> layout = font->font->shape("A");
    ASSERT_NE(layout, nullptr);
    ASSERT_FALSE(layout->glyphs.empty());

    vec2 origin;
    vec2 size;
    font->font->extents(layout->glyphs.front().id, 1.0f, origin, size);
    EXPECT_TRUE(std::isfinite(origin.x));
    EXPECT_TRUE(std::isfinite(origin.y));
    EXPECT_GT(size.x, 0.0);
    EXPECT_GT(size.y, 0.0);
}

TEST(SystemFontIntegrationTest, DistinguishesOutlineAndColorGlyphs) {
    px_font_t* font = px_create_font("system", 12.0f);
    ASSERT_NE(font, nullptr);

    const std::unique_ptr<fx_layout> outline = font->font->shape("A");
    ASSERT_NE(outline, nullptr);
    ASSERT_FALSE(outline->glyphs.empty());
    EXPECT_FALSE(font->font->is_color_glyph(outline->glyphs.front().id));

    const std::unique_ptr<fx_layout> colored = font->font->shape(U"\U0001f600");
    ASSERT_NE(colored, nullptr);
    ASSERT_FALSE(colored->glyphs.empty());
    EXPECT_TRUE(font->font->is_color_glyph(colored->glyphs.front().id));
}
#endif

void Utf8AndUtf32DrawingProduceEquivalentLayouts(const std::vector<char32_t>& codepoints) {
    const std::u32string utf32(codepoints.begin(), codepoints.end());
    const std::string utf8 = encode_utf8(utf32);
    shaper_fixture utf8_fixture;
    shaper_fixture utf32_fixture;
    recording_context utf8_context;
    recording_context utf32_context;

    utf8_fixture.shaper.draw_string(&utf8_context, 3.5, 7.25, {}, utf8, false, {}, false, 0.0f,
                                    0.0f);
    utf32_fixture.shaper.draw_string(&utf32_context, 3.5, 7.25, {}, utf32, false, {}, false, 0.0f,
                                     0.0f);

    // UTF-8 layouts report byte offsets while UTF-32 layouts report codepoint offsets.
    convert_utf8_clusters_to_codepoint_indices(&utf8_context, utf8);
    expect_equivalent_draws(utf8_context.draws, utf32_context.draws);
}
FUZZ_TEST(GraphemeShaperFuzzTest, Utf8AndUtf32DrawingProduceEquivalentLayouts)
    .WithDomains(shaper_text());

void OrdinaryTextDrawingAdvanceMatchesMeasurement(const std::vector<char32_t>& codepoints) {
    const std::u32string text(codepoints.begin(), codepoints.end());
    shaper_fixture fixture;
    const float measured = fixture.shaper.measure_string(text);
    recording_context context;
    constexpr double kOriginX = 3.5;

    fixture.shaper.draw_string(&context, kOriginX, 7.25, {}, text, false, {}, false, 0.0f, 0.0f);

    EXPECT_TRUE(std::isfinite(measured));
    EXPECT_GE(measured, 0.0f);
    EXPECT_FLOAT_EQ(static_cast<float>(drawn_advance(context, kOriginX)), measured);

    double expected_x = kOriginX;
    for (const recorded_draw& draw : context.draws) {
        EXPECT_DOUBLE_EQ(draw.position.x, expected_x);
        EXPECT_FALSE(draw.layout.glyphs.empty());
        EXPECT_TRUE(std::isfinite(draw.layout.advance));
        EXPECT_GE(draw.layout.advance, 0.0f);
        expected_x += draw.layout.advance;
    }
}
FUZZ_TEST(GraphemeShaperFuzzTest, OrdinaryTextDrawingAdvanceMatchesMeasurement)
    .WithDomains(ordinary_shaper_text());

void RepeatedDrawingReusesCachedLayouts(const std::vector<char32_t>& codepoints) {
    const std::u32string text(codepoints.begin(), codepoints.end());
    shaper_fixture fixture;
    recording_context first_context;
    recording_context second_context;

    fixture.shaper.draw_string(&first_context, 0.0, 0.0, {}, text, false, {}, false, 0.0f, 0.0f);
    const size_t shaped_count = fixture.fake->get_shaped_texts().size();
    fixture.shaper.draw_string(&second_context, 0.0, 0.0, {}, text, false, {}, false, 0.0f, 0.0f);

    EXPECT_EQ(fixture.fake->get_shaped_texts().size(), shaped_count);
    expect_equivalent_draws(first_context.draws, second_context.draws);
}
FUZZ_TEST(GraphemeShaperFuzzTest, RepeatedDrawingReusesCachedLayouts).WithDomains(shaper_text());

}  // namespace

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    fuzztest::ParseAbslFlags(argc, argv);
    if (std::getenv("RUN_FUZZTESTS")) {
        fuzztest::InitFuzzTest(&argc, &argv);
    }
    return RUN_ALL_TESTS();
}
