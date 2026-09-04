#include "experiments/platform/ui/retained_text.h"

namespace {

class retaining_render_context final : public px_render_context {
public:
    explicit retaining_render_context(retained_text* output) : output_(output) {}

    void draw_rect(rect, fill_mode) override {}

    void draw_shaped_text(px_font_t*, vec2 position, color, fx_layout* layout, bool) override {
        if (layout) {
            output_->batches.push_back({.x_offset = position.x, .layout = *layout});
        }
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

private:
    retained_text* output_;
};

void set_advance(retained_text* text) {
    if (!text->batches.empty()) {
        const retained_text_batch& last = text->batches.back();
        text->advance = last.x_offset + static_cast<double>(last.layout.advance);
    }
}

}  // namespace

retained_text prepare_retained_text(grapheme_shaper* shaper, std::string_view utf8) {
    retained_text result;
    if (!shaper) {
        return result;
    }

    retaining_render_context context(&result);
    shaper->draw_string(&context, 0.0, 0.0, {}, utf8, false, {}, false, 0.0f, 0.0f);
    set_advance(&result);
    return result;
}

retained_text prepare_retained_text(grapheme_shaper* shaper, std::u32string_view utf32) {
    retained_text result;
    if (!shaper) {
        return result;
    }

    retaining_render_context context(&result);
    shaper->draw_string(&context, 0.0, 0.0, {}, utf32, false, {}, false, 0.0f, 0.0f);
    set_advance(&result);
    return result;
}

retained_text prepare_retained_text(px_font_t* font, std::string_view utf8) {
    retained_text result;
    if (!font || utf8.empty()) {
        return result;
    }

    retaining_render_context context(&result);
    context.draw_text(font, {}, {}, utf8, false);
    set_advance(&result);
    return result;
}

void draw_retained_text(px_render_context* context,
                        px_font_t* font,
                        vec2 position,
                        color value,
                        retained_text* text,
                        bool subpixel_positioning) {
    if (!context || !text) {
        return;
    }
    for (retained_text_batch& batch : text->batches) {
        context->draw_shaped_text(font, {position.x + batch.x_offset, position.y}, value,
                                  &batch.layout, subpixel_positioning);
    }
}
