// The OpenGL implementation of px_render_context.
//
// The class boundary and responsibilities mirror Sublime Text's gl_render_context. Raw GL object
// ownership lives in a process-wide gl_render_state; this short-lived object contains only the
// state of one paint traversal: device size, dirty/control clip, DPI transform, state stack and an
// optional rectangle batch.

#pragma once

#include <deque>
#include <memory>
#include <vector>

#include "experiments/platform/px/px.h"

class gl_rect_batch;

class gl_render_context final : public px_render_context {
public:
    using px_render_context::draw_rect;

    // `dirty` is in logical window coordinates. The constructor establishes both ST's coarse
    // bounding-union scissor and an exact stencil mask for disjoint dirty rectangles.
    gl_render_context(vec2 device_size,
                      double dpi_scale,
                      const rect* dirty,
                      int dirty_count,
                      bool has_stencil = true);
    ~gl_render_context() override;

    gl_render_context(const gl_render_context&) = delete;
    gl_render_context& operator=(const gl_render_context&) = delete;

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

    bool supports_batching() const override { return true; }
    void begin_text_batch() override;
    void end_text_batch() override;
    void begin_rect_batch() override;
    void end_rect_batch() override;
    void begin_line_batch() override;
    void end_line_batch() override;

    // Submit pending batches and restore any unmatched pushed states. Platform backends call this
    // after paint() and before presenting, so a client that forgets an end/pop does not silently
    // defer its drawing until the next frame. The destructor calls it as a final safety net.
    void finish();

    // The logical bounding union supplied to px_window_event_handler::paint in ST.
    rect paint_bounds() const { return paint_bounds_; }

    // ST replaces more than 128 dirty rectangles with their union before rendering. Platform paint
    // paths call this after any framebuffer-resize full invalidation and before constructing us.
    static rect normalize_dirty_rects(std::vector<rect>* dirty, rect window_bounds);

    // The rasterizer comparison swaps thousands of independent pages through one persistent
    // window. Its reference renderer starts each page with a fresh atlas, so request the same on
    // the next text draw. Deferred because callers do not own the current GL context.
    static void reset_glyph_atlas_for_testing();

private:
    struct saved_state {
        vec2 translation;
        vec2 scale;
        recti clip;
        bool has_batch_state = false;
        std::unique_ptr<gl_rect_batch> rect_batch;
    };

    recti device_rect(rect area) const;
    rect transformed_rect(rect area) const;
    void apply_clip();
    void establish_dirty_mask(const rect* dirty, int dirty_count);

    vec2 device_size_;
    rect paint_bounds_;
    recti clip_;
    double dpi_scale_ = 1.0;
    bool has_stencil_ = true;
    vec2 translation_;
    vec2 scale_{1.0, 1.0};
    std::unique_ptr<gl_rect_batch> rect_batch_;
    std::deque<saved_state> state_stack_;
};
