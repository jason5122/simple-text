// The Metal implementation of px_render_context.
//
// Structured like gl_render_context: persistent GPU objects (device, queue, pipelines, glyph
// atlases, upload buffers) live in a process-wide state, and this short-lived object holds only one
// paint traversal's worth of state -- clip, DPI transform, state stack and an optional rectangle
// batch. Where the GL context draws into whatever framebuffer is bound, the Metal context records
// into a metal_frame that the platform layer opens around the paint and presents afterwards.
//
// The header is plain C++ so portable code (the conformance harness, in particular) can name the
// class; everything that touches Metal types is behind __OBJC__ at the bottom.

#pragma once

#include "px/px.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

class metal_rect_batch;

// One paint's command stream and render target: a command buffer, the render pass into the
// persistent color (and optional stencil) texture, and a bump allocator for per-frame instance
// data. The platform layer opens it with metal_frame_begin, hands it to a metal_render_context, and
// once the context has finished, appends its own presentation work to the same command buffer.
class metal_frame {
public:
    struct impl;

    explicit metal_frame(std::unique_ptr<impl> impl);
    ~metal_frame();

    metal_frame(const metal_frame&) = delete;
    metal_frame& operator=(const metal_frame&) = delete;

    vec2 device_size() const;
    bool has_stencil() const;

    // Ends the render pass. Idempotent; metal_render_context::finish calls it, and the destructor
    // calls it as a safety net. No drawing is possible afterwards.
    void end();

    impl& internal() { return *impl_; }

private:
    std::unique_ptr<impl> impl_;
};

class metal_render_context final : public px_render_context {
public:
    using px_render_context::draw_rect;

    // `dirty` is in logical window coordinates. When the frame has a stencil attachment the
    // constructor rasterizes an exact mask of the disjoint dirty rectangles into it, and every
    // later draw is stencil-tested against that mask; the scissor is the coarser bounding union.
    // Without a stencil only the scissor applies.
    metal_render_context(metal_frame* frame,
                         double dpi_scale,
                         const rect* dirty,
                         int dirty_count,
                         uint32_t subpixel_order = 0);
    ~metal_render_context() override;

    metal_render_context(const metal_render_context&) = delete;
    metal_render_context& operator=(const metal_render_context&) = delete;

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

    // Submits pending batches, restores any unmatched pushed states and ends the frame's render
    // pass. Platform backends call this after paint() and before presenting; the destructor calls
    // it as a final safety net.
    void finish();

    // The logical bounding union supplied to px_window_event_handler::paint.
    rect paint_bounds() const { return paint_bounds_; }

    // ST replaces more than 128 dirty rectangles with their union before rendering. Platform paint
    // paths call this after any framebuffer-resize full invalidation and before constructing us.
    static rect normalize_dirty_rects(std::vector<rect>* dirty, rect window_bounds);

    // Discards atlas placements and pages on the next text draw, so a test sees the same first-use
    // upload order every time.
    static void reset_glyph_atlas_for_testing();

private:
    struct saved_state {
        vec2 translation;
        vec2 scale;
        recti clip;
        bool has_batch_state = false;
        std::unique_ptr<metal_rect_batch> rect_batch;
    };

    recti device_rect(rect area) const;
    rect transformed_rect(rect area) const;
    void apply_clip();
    void establish_dirty_mask(const rect* dirty, int dirty_count);

    metal_frame* frame_ = nullptr;
    vec2 device_size_;
    rect paint_bounds_;
    recti clip_;
    double dpi_scale_ = 1.0;
    uint32_t subpixel_order_ = 0;
    bool has_stencil_ = false;
    vec2 translation_;
    vec2 scale_{1.0, 1.0};
    std::unique_ptr<metal_rect_batch> rect_batch_;
    std::deque<saved_state> state_stack_;
};

#ifdef __OBJC__

#import <Metal/Metal.h>

// The process-wide device and queue, created on first use and never released, like the shared CGL
// context in px_gl_layer.mm. nil when the machine has no Metal device.
id<MTLDevice> metal_render_device();
id<MTLCommandQueue> metal_render_command_queue();

// Opens a render pass on `command_buffer` into `color`, a BGRA8Unorm render target at least
// `device_size` pixels in each dimension. Its contents are loaded, not cleared: dirty regions are
// repainted on top of the previous frame. `stencil` may be nil, in which case dirty rectangles
// are honored only through the coarse scissor; when present it is cleared at the start of the
// pass. Returns null if the pass could not be created.
std::unique_ptr<metal_frame> metal_frame_begin(id<MTLCommandBuffer> command_buffer,
                                               id<MTLTexture> color,
                                               id<MTLTexture> stencil,
                                               vec2 device_size);

#endif
