#include "experiments/platform/px/px.h"

#include "experiments/platform/px/px_font_private.h"

px_font_t* px_create_font(const char*, float, uint32_t) { return nullptr; }

float px_font_em_width(px_font_t*) { return 0.0f; }

bool px_font_is_monospace(px_font_t*) { return false; }

px_font_metrics px_font_get_metrics(px_font_t*) { return {}; }

std::vector<fx_layout_batch> px_shape_text(px_font_t*, std::string_view) { return {}; }

void px_render_context::draw_text(px_font_t*, vec2, fcolor, std::string_view, bool) {}
