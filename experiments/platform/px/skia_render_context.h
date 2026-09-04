// The CPU implementation of px_render_context.
//
// Like Sublime Text's skia_render_context, this is a short-lived view over caller-owned BGRA
// pixels. Skia handles ordinary 2D geometry while text continues to use px's fx shaping and glyph
// rasterization pipeline.

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "experiments/platform/px/px.h"

struct px_pixel_buffer {
    void* pixels = nullptr;
    int width = 0;
    int height = 0;
    size_t row_bytes = 0;
};

class skia_render_context final : public px_render_context {
public:
    using px_render_context::draw_rect;

    skia_render_context(px_pixel_buffer buffer, recti clip, double dpi_scale);
    ~skia_render_context() override;

    skia_render_context(const skia_render_context&) = delete;
    skia_render_context& operator=(const skia_render_context&) = delete;

    bool valid() const;

    void draw_rect(rect area, fill_mode fill) override;
    void draw_shaped_text(px_font_t* font,
                          vec2 position,
                          color value,
                          fx_layout* layout,
                          bool subpixel_positioning) override;

    void translate(double x, double y) override;
    void scale(double x, double y) override;
    void restrict_clip_rect(rect area) override;
    void push_state(bool preserve_batch) override;
    void pop_state() override;

    vec2 get_translation() override { return translation_; }
    vec2 get_scale() override { return scale_; }
    recti get_clip_rect() override { return clip_; }
    double dpi_scale_factor() override { return dpi_scale_; }

private:
    struct saved_state {
        vec2 translation;
        vec2 scale;
        recti clip;
    };

    class impl;

    rect transformed_rect(rect area) const;
    recti device_rect(rect area) const;

    px_pixel_buffer buffer_;
    recti clip_;
    double dpi_scale_ = 1.0;
    vec2 translation_;
    vec2 scale_{1.0, 1.0};
    std::unique_ptr<impl> impl_;
    std::vector<saved_state> state_stack_;
};
