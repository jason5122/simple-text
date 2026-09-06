#pragma once

#include "experiments/platform/px/px.h"
#include "fx/fx.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

// The four policy bits passed to Sublime's grapheme_shaper(px_font_t*, unsigned). The first two
// are the two increasingly broad draw_unicode_white_space modes, the third enables bidi controls,
// and the fourth selects mnemonic names instead of hexadecimal control labels.
enum grapheme_shaper_flag : uint32_t {
    GRAPHEME_SHAPER_DRAW_COMMON_WHITESPACE = 1u << 0,
    GRAPHEME_SHAPER_DRAW_ALL_WHITESPACE = 1u << 1,
    GRAPHEME_SHAPER_DRAW_BIDI = 1u << 2,
    GRAPHEME_SHAPER_USE_CONTROL_NAMES = 1u << 3,
};

// Renderer-neutral text policy above fx_font's native Core Text/DirectWrite shaping. One instance
// belongs to one px_font_t and caches the fx_layout for code points and grapheme/operator strings.
// It does not rasterize: drawing ends at px_render_context::draw_shaped_text().
class grapheme_shaper {
public:
    explicit grapheme_shaper(px_font_t* font, uint32_t flags = 0);
    ~grapheme_shaper();

    grapheme_shaper(const grapheme_shaper&) = delete;
    grapheme_shaper& operator=(const grapheme_shaper&) = delete;

    float measure_string(std::string_view utf8);
    float measure_string(std::u32string_view utf32);
    float measure_glyph(char32_t codepoint);
    float measure_glyph(std::string_view utf8);
    float measure_glyph(std::u32string_view utf32);

    fx_layout* find_layout(char32_t codepoint);
    fx_layout* find_layout(std::string_view utf8);
    fx_layout* find_layout(std::u32string_view utf32);

    void draw_string(px_render_context* context,
                     double x,
                     double y,
                     color value,
                     std::string_view utf8,
                     bool draw_spaces,
                     color space_color,
                     bool fade,
                     float fade_start,
                     float fade_end);
    void draw_string(px_render_context* context,
                     double x,
                     double y,
                     color value,
                     std::u32string_view utf32,
                     bool draw_spaces,
                     color space_color,
                     bool clip,
                     float clip_start,
                     float clip_end);

    // ST uses this process-lifetime map for UI consumers.
    static grapheme_shaper* instance(px_font_t* font);

private:
    template <typename Text, typename Decoder, typename Callback>
    void draw_string_impl(Callback& callback,
                          double x,
                          double y,
                          color ordinary_color,
                          Text text,
                          bool draw_spaces,
                          color space_color);

    bool should_draw_as_control(char32_t codepoint) const;
    std::unique_ptr<fx_layout> shape_control(char32_t codepoint);

    px_font_t* font_ = nullptr;
    uint32_t flags_ = 0;
    bool monospace_ = false;
    std::map<char32_t, std::unique_ptr<fx_layout>> codepoint_layouts_;
    std::map<std::string, std::unique_ptr<fx_layout>, std::less<>> utf8_layouts_;
    std::map<std::u32string, std::unique_ptr<fx_layout>, std::less<>> utf32_layouts_;
    static std::map<px_font_t*, std::unique_ptr<grapheme_shaper>> s_cache;
};
