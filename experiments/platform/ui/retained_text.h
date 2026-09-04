#pragma once

#include "experiments/platform/px/grapheme_shaper.h"
#include "experiments/platform/px/px.h"
#include "fx/fx.h"

#include <string_view>
#include <vector>

// Local retained adapter for demos, benchmarks, and conformance harnesses. Sublime's
// grapheme_shaper emits temporary layouts directly to a render context and does not expose these
// types.
struct retained_text_batch {
    double x_offset = 0.0;
    fx_layout layout;
};

struct retained_text {
    std::vector<retained_text_batch> batches;
    double advance = 0.0;
};

retained_text prepare_retained_text(grapheme_shaper* shaper, std::string_view utf8);
retained_text prepare_retained_text(grapheme_shaper* shaper, std::u32string_view utf32);

// Retains one whole native layout, matching px_render_context::draw_text rather than applying
// grapheme_shaper policy.
retained_text prepare_retained_text(px_font_t* font, std::string_view utf8);

void draw_retained_text(px_render_context* context,
                        px_font_t* font,
                        vec2 position,
                        color value,
                        retained_text* text,
                        bool subpixel_positioning = true);
