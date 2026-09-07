#include "px/metal_render_context.h"
#include "px/px_font_internal.h"
#import <Metal/Metal.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <tuple>
#include <utility>
#include <vector>

namespace {

constexpr int kMaximumDirtyRects = 128;
constexpr MTLPixelFormat kColorFormat = MTLPixelFormatBGRA8Unorm;
constexpr MTLPixelFormat kStencilFormat = MTLPixelFormatStencil8;

// Per-frame instance data is bump-allocated out of shared buffers this large, pooled across frames
// and returned by the command buffer's completion handler. ST gets the same effect from its pair of
// alternating dynamic GL buffers.
constexpr size_t kUploadChunkBytes = 256 * 1024;
constexpr size_t kUploadPoolLimit = 32;
constexpr size_t kUploadAlignment = 256;

rect intersect_rect(rect a, rect b) {
    const double left = std::max(a.x, b.x);
    const double top = std::max(a.y, b.y);
    const double right = std::min(a.right(), b.right());
    const double bottom = std::min(a.bottom(), b.bottom());
    return rect{left, top, std::max(0.0, right - left), std::max(0.0, bottom - top)};
}

rect union_rect(rect a, rect b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const double left = std::min(a.x, b.x);
    const double top = std::min(a.y, b.y);
    const double right = std::max(a.right(), b.right());
    const double bottom = std::max(a.bottom(), b.bottom());
    return rect{left, top, right - left, bottom - top};
}

recti intersect_recti(recti a, recti b) {
    recti result{std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right),
                 std::min(a.bottom, b.bottom)};
    if (result.empty()) {
        result.right = result.left;
        result.bottom = result.top;
    }
    return result;
}

// Layouts shared with metal_shaders.metal.
struct rect_instance {
    float x;
    float y;
    float width;
    float height;
    float r;
    float g;
    float b;
    float a;
};
static_assert(sizeof(rect_instance) == 2 * sizeof(float) * 4);

struct glyph_instance_data {
    float r, g, b, a;
    float x, y, width, height;
    // The final lanes hold fade or horizontal-clip bounds in those shader variants.
    float u, v, effect_start, effect_end;
};
static_assert(sizeof(glyph_instance_data) == 3 * sizeof(float) * 4);

struct glyph_vertex_uniforms {
    float viewport[2];
    float texture_size;
    float padding;
};

struct glyph_fragment_uniforms {
    uint32_t colored;
    uint32_t alternate;
};

constexpr const char* kShaderSource =
#include "px/metal_shaders.metal"
    ;

enum class blend_mode {
    none,
    premultiplied,  // ONE, ONE_MINUS_SRC_ALPHA: solid rectangles and colored glyphs
    dual_source,    // SRC1_COLOR, ONE_MINUS_SRC1_COLOR: per-channel glyph coverage
};

id<MTLRenderPipelineState> make_pipeline(id<MTLDevice> device,
                                         id<MTLLibrary> library,
                                         NSString* vertex_name,
                                         NSString* fragment_name,
                                         blend_mode blend,
                                         bool color_writes,
                                         bool has_stencil,
                                         NSString* label) {
    id<MTLFunction> vertex = [library newFunctionWithName:vertex_name];
    id<MTLFunction> fragment = [library newFunctionWithName:fragment_name];
    if (!vertex || !fragment) {
        std::fprintf(stderr, "px: metal_render_context is missing shader functions for %s\n",
                     label.UTF8String);
        return nil;
    }

    MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.label = label;
    descriptor.vertexFunction = vertex;
    descriptor.fragmentFunction = fragment;
    descriptor.stencilAttachmentPixelFormat = has_stencil ? kStencilFormat : MTLPixelFormatInvalid;

    MTLRenderPipelineColorAttachmentDescriptor* color = descriptor.colorAttachments[0];
    color.pixelFormat = kColorFormat;
    color.writeMask = color_writes ? MTLColorWriteMaskAll : MTLColorWriteMaskNone;
    color.rgbBlendOperation = MTLBlendOperationAdd;
    color.alphaBlendOperation = MTLBlendOperationAdd;
    switch (blend) {
    case blend_mode::none:
        color.blendingEnabled = NO;
        break;
    case blend_mode::premultiplied:
        color.blendingEnabled = YES;
        color.sourceRGBBlendFactor = MTLBlendFactorOne;
        color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        color.sourceAlphaBlendFactor = MTLBlendFactorOne;
        color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        break;
    case blend_mode::dual_source:
        color.blendingEnabled = YES;
        color.sourceRGBBlendFactor = MTLBlendFactorSource1Color;
        color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSource1Color;
        color.sourceAlphaBlendFactor = MTLBlendFactorSource1Alpha;
        color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSource1Alpha;
        break;
    }

    NSError* error = nil;
    id<MTLRenderPipelineState> state = [device newRenderPipelineStateWithDescriptor:descriptor
                                                                              error:&error];
    if (!state) {
        std::fprintf(stderr, "px: metal_render_context pipeline %s failed: %s\n",
                     label.UTF8String, error.localizedDescription.UTF8String);
    }
    return state;
}

id<MTLDepthStencilState> make_stencil_state(id<MTLDevice> device,
                                            MTLCompareFunction compare,
                                            MTLStencilOperation operation,
                                            uint32_t write_mask,
                                            NSString* label) {
    MTLStencilDescriptor* stencil = [[MTLStencilDescriptor alloc] init];
    stencil.stencilCompareFunction = compare;
    stencil.stencilFailureOperation = operation;
    stencil.depthFailureOperation = operation;
    stencil.depthStencilPassOperation = operation;
    stencil.readMask = 0xff;
    stencil.writeMask = write_mask;

    MTLDepthStencilDescriptor* descriptor = [[MTLDepthStencilDescriptor alloc] init];
    descriptor.label = label;
    descriptor.frontFaceStencil = stencil;
    descriptor.backFaceStencil = stencil;
    return [device newDepthStencilStateWithDescriptor:descriptor];
}

// Persistent Metal objects: the device and queue, one pipeline per (primitive, blend, stencil)
// combination, the stencil states, and the upload-buffer pool. Process-lifetime for the same reason
// as ST's g_gl_render_state: every window shares them, and tearing them down after the windows
// would be pointless.
class metal_device_state {
public:
    id<MTLDevice> device() {
        ensure_device();
        return device_;
    }

    id<MTLCommandQueue> queue() {
        ensure_device();
        return queue_;
    }

    bool usable() {
        ensure_pipelines();
        return pipelines_ready_;
    }

    id<MTLRenderPipelineState> rect_pipeline(bool has_stencil) {
        return rect_pipelines_[has_stencil ? 1 : 0];
    }

    id<MTLRenderPipelineState> mask_pipeline() { return mask_pipeline_; }

    id<MTLRenderPipelineState> glyph_pipeline(bool colored, bool has_stencil) {
        return colored ? glyph_color_pipelines_[has_stencil ? 1 : 0]
                       : glyph_mono_pipelines_[has_stencil ? 1 : 0];
    }

    id<MTLDepthStencilState> stencil_write_state() { return stencil_write_; }
    id<MTLDepthStencilState> stencil_test_state() { return stencil_test_; }

    // Smallest pooled buffer that fits, or a fresh one.
    id<MTLBuffer> acquire_buffer(size_t size) {
        {
            const std::lock_guard lock(pool_mutex_);
            auto best = pool_.end();
            for (auto it = pool_.begin(); it != pool_.end(); ++it) {
                const NSUInteger length = [*it length];
                if (length >= size && (best == pool_.end() || length < [*best length])) {
                    best = it;
                }
            }
            if (best != pool_.end()) {
                id<MTLBuffer> buffer = *best;
                pool_.erase(best);
                return buffer;
            }
        }

        ensure_device();
        if (!device_) {
            return nil;
        }
        id<MTLBuffer> buffer = [device_ newBufferWithLength:std::max(size, kUploadChunkBytes)
                                                    options:MTLResourceStorageModeShared];
        buffer.label = @"px instances";
        return buffer;
    }

    // Called from the command buffer's completion handler, so on a Metal worker thread.
    void recycle_buffer(id<MTLBuffer> buffer) {
        if (!buffer) {
            return;
        }
        const std::lock_guard lock(pool_mutex_);
        if (pool_.size() < kUploadPoolLimit) {
            pool_.push_back(buffer);
        }
    }

private:
    void ensure_device() {
        if (device_probed_) {
            return;
        }
        device_probed_ = true;
        device_ = MTLCreateSystemDefaultDevice();
        if (!device_) {
            std::fprintf(stderr, "px: no Metal device is available\n");
            return;
        }
        queue_ = [device_ newCommandQueue];
        queue_.label = @"px";
    }

    void ensure_pipelines() {
        if (pipelines_probed_) {
            return;
        }
        pipelines_probed_ = true;
        ensure_device();
        if (!device_) {
            return;
        }

        NSError* error = nil;
        MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
        id<MTLLibrary> library = [device_ newLibraryWithSource:@(kShaderSource)
                                                       options:options
                                                         error:&error];
        if (!library) {
            std::fprintf(stderr, "px: metal_render_context shader compile failed: %s\n",
                         error.localizedDescription.UTF8String);
            return;
        }

        for (int i = 0; i < 2; ++i) {
            const bool has_stencil = i == 1;
            rect_pipelines_[i] =
                make_pipeline(device_, library, @"px_rect_vertex", @"px_rect_fragment",
                              blend_mode::premultiplied, true, has_stencil,
                              has_stencil ? @"px rect (stencil)" : @"px rect");
            glyph_mono_pipelines_[i] =
                make_pipeline(device_, library, @"px_glyph_vertex", @"px_glyph_fragment",
                              blend_mode::dual_source, true, has_stencil,
                              has_stencil ? @"px glyph (stencil)" : @"px glyph");
            glyph_color_pipelines_[i] =
                make_pipeline(device_, library, @"px_glyph_vertex", @"px_glyph_fragment",
                              blend_mode::premultiplied, true, has_stencil,
                              has_stencil ? @"px colored glyph (stencil)" : @"px colored glyph");
        }
        // The dirty mask writes stencil only. ST draws it with glColorMask all-false; Metal
        // expresses that as a pipeline whose color write mask is empty.
        mask_pipeline_ = make_pipeline(device_, library, @"px_rect_vertex", @"px_rect_fragment",
                                       blend_mode::none, false, true, @"px dirty mask");
        stencil_write_ = make_stencil_state(device_, MTLCompareFunctionAlways,
                                            MTLStencilOperationReplace, 0xff, @"px stencil write");
        stencil_test_ = make_stencil_state(device_, MTLCompareFunctionEqual,
                                           MTLStencilOperationKeep, 0x00, @"px stencil test");

        pipelines_ready_ = rect_pipelines_[0] && rect_pipelines_[1] && glyph_mono_pipelines_[0] &&
                           glyph_mono_pipelines_[1] && glyph_color_pipelines_[0] &&
                           glyph_color_pipelines_[1] && mask_pipeline_ && stencil_write_ &&
                           stencil_test_;
    }

    bool device_probed_ = false;
    bool pipelines_probed_ = false;
    bool pipelines_ready_ = false;
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> queue_ = nil;
    id<MTLRenderPipelineState> rect_pipelines_[2] = {nil, nil};
    id<MTLRenderPipelineState> glyph_mono_pipelines_[2] = {nil, nil};
    id<MTLRenderPipelineState> glyph_color_pipelines_[2] = {nil, nil};
    id<MTLRenderPipelineState> mask_pipeline_ = nil;
    id<MTLDepthStencilState> stencil_write_ = nil;
    id<MTLDepthStencilState> stencil_test_ = nil;
    std::mutex pool_mutex_;
    std::vector<id<MTLBuffer>> pool_;
};

metal_device_state& device_state() {
    // Intentionally process-lifetime.
    static auto* state = new metal_device_state;
    return *state;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// metal_frame
// ─────────────────────────────────────────────────────────────────────────────────────────────────

struct metal_frame::impl {
    id<MTLCommandBuffer> command_buffer = nil;
    id<MTLRenderCommandEncoder> encoder = nil;
    vec2 device_size;
    bool has_stencil = false;
    bool ended = false;

    // Bump allocator over pooled shared buffers.
    std::vector<id<MTLBuffer>> buffers;
    size_t offset = 0;

    // Current encoder bindings, tracked to skip redundant state changes.
    id<MTLRenderPipelineState> pipeline = nil;
    id<MTLDepthStencilState> depth_stencil = nil;

    struct allocation {
        id<MTLBuffer> buffer = nil;
        size_t offset = 0;
        void* pointer = nullptr;
    };

    allocation allocate(size_t size) {
        if (buffers.empty() || offset + size > [buffers.back() length]) {
            id<MTLBuffer> buffer = device_state().acquire_buffer(size);
            if (!buffer) {
                return {};
            }
            buffers.push_back(buffer);
            offset = 0;
        }
        id<MTLBuffer> buffer = buffers.back();
        allocation result{buffer, offset, static_cast<char*>(buffer.contents) + offset};
        offset = (offset + size + kUploadAlignment - 1) & ~(kUploadAlignment - 1);
        return result;
    }

    void set_pipeline(id<MTLRenderPipelineState> state) {
        if (state != pipeline) {
            pipeline = state;
            [encoder setRenderPipelineState:state];
        }
    }

    void set_depth_stencil(id<MTLDepthStencilState> state) {
        if (state != depth_stencil) {
            depth_stencil = state;
            [encoder setDepthStencilState:state];
        }
    }

    void end() {
        if (ended) {
            return;
        }
        ended = true;
        [encoder endEncoding];
        encoder = nil;
        pipeline = nil;
        depth_stencil = nil;
        if (!buffers.empty()) {
            // The GPU may still be reading these when the layer commits. Hand them back only once
            // the command buffer has retired.
            const std::vector<id<MTLBuffer>> recycled = std::move(buffers);
            [command_buffer addCompletedHandler:^(id<MTLCommandBuffer>) {
              for (id<MTLBuffer> buffer : recycled) {
                  device_state().recycle_buffer(buffer);
              }
            }];
        }
        buffers.clear();
        command_buffer = nil;
    }
};

metal_frame::metal_frame(std::unique_ptr<impl> impl) : impl_(std::move(impl)) {}

metal_frame::~metal_frame() { end(); }

vec2 metal_frame::device_size() const { return impl_->device_size; }

bool metal_frame::has_stencil() const { return impl_->has_stencil; }

void metal_frame::end() {
    if (impl_) {
        impl_->end();
    }
}

id<MTLDevice> metal_render_device() { return device_state().device(); }

id<MTLCommandQueue> metal_render_command_queue() { return device_state().queue(); }

std::unique_ptr<metal_frame> metal_frame_begin(id<MTLCommandBuffer> command_buffer,
                                               id<MTLTexture> color,
                                               id<MTLTexture> stencil,
                                               vec2 device_size) {
    const double width = std::floor(device_size.x);
    const double height = std::floor(device_size.y);
    if (!command_buffer || !color || width < 1.0 || height < 1.0 ||
        static_cast<NSUInteger>(width) > color.width ||
        static_cast<NSUInteger>(height) > color.height) {
        return nullptr;
    }

    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = color;
    pass.colorAttachments[0].loadAction = MTLLoadActionLoad;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    if (stencil) {
        pass.stencilAttachment.texture = stencil;
        pass.stencilAttachment.loadAction = MTLLoadActionClear;
        pass.stencilAttachment.clearStencil = 0;
        pass.stencilAttachment.storeAction = MTLStoreActionDontCare;
    }

    id<MTLRenderCommandEncoder> encoder = [command_buffer renderCommandEncoderWithDescriptor:pass];
    if (!encoder) {
        return nullptr;
    }
    encoder.label = @"px paint";
    const MTLViewport viewport{0.0, 0.0, width, height, 0.0, 1.0};
    [encoder setViewport:viewport];
    [encoder setStencilReferenceValue:1];

    auto state = std::make_unique<metal_frame::impl>();
    state->command_buffer = command_buffer;
    state->encoder = encoder;
    state->device_size = vec2{width, height};
    state->has_stencil = stencil != nil;
    return std::make_unique<metal_frame>(std::move(state));
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// Rectangle and glyph submission
// ─────────────────────────────────────────────────────────────────────────────────────────────────

namespace {

// One instanced draw. `mask` selects the color-write-disabled pipeline used for the dirty stencil.
void draw_rects(const std::vector<rect_instance>& instances, metal_frame* frame, bool mask) {
    if (instances.empty() || !frame || !device_state().usable()) return;
    metal_frame::impl& impl = frame->internal();
    if (!impl.encoder) return;

    const size_t bytes = instances.size() * sizeof(rect_instance);
    const metal_frame::impl::allocation allocation = impl.allocate(bytes);
    if (!allocation.pointer) return;
    std::memcpy(allocation.pointer, instances.data(), bytes);

    impl.set_pipeline(mask ? device_state().mask_pipeline()
                           : device_state().rect_pipeline(impl.has_stencil));
    const float viewport[2] = {static_cast<float>(impl.device_size.x),
                               static_cast<float>(impl.device_size.y)};
    [impl.encoder setVertexBuffer:allocation.buffer offset:allocation.offset atIndex:0];
    [impl.encoder setVertexBytes:viewport length:sizeof(viewport) atIndex:1];
    [impl.encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                     vertexStart:0
                     vertexCount:4
                   instanceCount:instances.size()];
}

struct glyph_atlas_key {
    const px_font_t* font = nullptr;
    uint32_t glyph = 0;
    int phase = 0;
    uint32_t scale_percent = 100;
    uint32_t subpixel_order = 0;
    bool alternate = false;

    auto operator<=>(const glyph_atlas_key&) const = default;
};

struct glyph_atlas_placement {
    float u = 0.0f;
    float v = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int page = -1;
    bool colored = false;
};

// Glyph atlases and the text batch. Same structure as the GL version: one atlas set per
// (font, raster scale), row-packed pages, and batches grouped by everything that has to be
// identical for two glyphs to share one draw call.
class metal_text_render_state {
public:
    void request_reset() { reset_requested_.store(true, std::memory_order_release); }

    void begin_batch(metal_frame* frame) {
        if (batch_depth_++ == 0) {
            batch_frame_ = frame;
            batch_groups_.clear();
        }
    }

    void end_batch() {
        if (batch_depth_ == 0 || --batch_depth_ != 0) {
            return;
        }
        flush_batch();
    }

    void finish_batch() {
        if (batch_depth_ == 0) {
            return;
        }
        batch_depth_ = 0;
        flush_batch();
        batch_frame_ = nullptr;
    }

    // Drops a batch whose frame no longer exists.
    void abandon_batch() {
        batch_depth_ = 0;
        batch_frame_ = nullptr;
        batch_groups_.clear();
    }

    void flush_pending_batch() {
        if (batch_depth_ != 0) {
            flush_batch();
        }
    }

    void draw(px_font_t* font,
              const fx_layout& layout,
              vec2 origin,
              fcolor color,
              vec2 translation,
              vec2 scale,
              metal_frame* frame,
              recti clip,
              bool subpixel_positioning,
              uint32_t subpixel_order) {
        if (!frame || !device_state().usable() || !font || !font->font || layout.glyphs.empty() ||
            clip.empty()) {
            return;
        }
        if (reset_requested_.exchange(false, std::memory_order_acq_rel)) {
            flush_batch();
            // Pages are dropped rather than cleared in place: a frame still in flight may be
            // sampling them, and its command buffer keeps the old textures alive until it retires.
            for (auto& [unused, atlas] : atlas_sets_) {
                atlas.placements.clear();
                atlas.pages.clear();
                atlas.active_page_count = 0;
            }
        }

        const float raster_scale = std::max(0.01f, static_cast<float>(std::abs(scale.x)));
        const uint32_t scale_percent =
            static_cast<uint32_t>(static_cast<double>(raster_scale) * 100.0);
        fx_glyph_cache& cache = font->glyph_cache(raster_scale);
        atlas_set& atlas = atlas_sets_[{font, scale_percent}];
        if (atlas.size == 0) atlas.size = atlas_size_for(*font, raster_scale);
        std::vector<texture_batch_group> immediate_groups;
        std::vector<texture_batch_group>& groups =
            batch_depth_ != 0 ? batch_groups_ : immediate_groups;
        const double device_origin_x = translation.x + origin.x * scale.x;
        const double device_origin_y = translation.y + origin.y * scale.y;
        const float lightness =
            (std::max({color.r, color.g, color.b}) + std::min({color.r, color.g, color.b})) * 0.5f;
        // Sublime selects the inverted glyph-cache polarity only for very light tints.
        const bool alternate = lightness > 0.75f;

        for (const fx_glyph& glyph : layout.glyphs) {
            const double x = device_origin_x + static_cast<double>(glyph.x_offset) * scale.x;
            const double y = device_origin_y + static_cast<double>(glyph.y_offset) * scale.y;
            const int device_x = static_cast<int>(std::floor(x));
            const double fraction = x - std::floor(x);
            const int phase =
                subpixel_positioning
                    ? std::clamp(static_cast<int>(fraction * fx_glyph_cache::phase_count), 0,
                                 static_cast<int>(fx_glyph_cache::phase_count) - 1)
                    : 0;
            const glyph_atlas_key key{font,          glyph.id,       phase,
                                      scale_percent, subpixel_order, alternate};
            const fx_glyph_cache::glyph_data& data =
                cache.lookup_glyph_data(glyph.id, subpixel_order, alternate);
            ensure_phase_pages(&atlas, key, data);
            const fx_glyph_cache::glyph_phase& glyph_phase = data.phase_at(phase);
            const glyph_atlas_placement* placement = place(&atlas, key, glyph_phase, data.colored);
            if (!placement) {
                continue;
            }

            const float instance_x = static_cast<float>(device_x + glyph_phase.bearing_x);
            // Nearest with half-pixel ties toward the top, the top-left expression of ST's frinta
            // rounding in GL's bottom-left space.
            const float instance_y =
                static_cast<float>(std::ceil(y - 0.5) + glyph_phase.bearing_y);
            const float instance_width = static_cast<float>(glyph_phase.width);
            const float instance_height = static_cast<float>(glyph_phase.height);
            if (instance_x + instance_width <= static_cast<float>(clip.left) ||
                instance_x >= static_cast<float>(clip.right) ||
                instance_y + instance_height <= static_cast<float>(clip.top) ||
                instance_y >= static_cast<float>(clip.bottom)) {
                continue;
            }

            const glyph_instance_data instance{
                .r = color.r,
                .g = color.g,
                .b = color.b,
                .a = color.a,
                .x = instance_x,
                .y = instance_y,
                .width = placement->width,
                .height = placement->height,
                .u = placement->u,
                .v = placement->v,
                .effect_start = 0.0f,
                .effect_end = 0.0f,
            };
            add_to_groups(&groups,
                          {.atlas = &atlas,
                           .page = placement->page,
                           .colored = placement->colored,
                           .alternate = alternate},
                          instance);
        }

        if (batch_depth_ == 0) {
            render_groups(immediate_groups, frame);
        }
    }

private:
    struct atlas_page {
        id<MTLTexture> texture = nil;
        int row_x = 0;
        int row_y = 0;
        int row_height = 0;
    };

    struct atlas_set {
        int size = 0;
        std::map<glyph_atlas_key, glyph_atlas_placement> placements;
        std::vector<atlas_page> pages;
        size_t active_page_count = 0;
    };

    struct batch_key {
        const atlas_set* atlas = nullptr;
        int page = -1;
        bool colored = false;
        bool alternate = false;

        bool operator==(const batch_key&) const = default;
    };

    struct texture_batch_group {
        batch_key key;
        std::vector<glyph_instance_data> instances;
    };

    static void add_to_groups(std::vector<texture_batch_group>* groups,
                              const batch_key& key,
                              glyph_instance_data instance) {
        auto found = std::find_if(
            groups->begin(), groups->end(),
            [&key](const texture_batch_group& group) { return group.key == key; });
        if (found == groups->end()) {
            groups->push_back({.key = key});
            found = std::prev(groups->end());
        }
        found->instances.push_back(instance);
    }

    void flush_batch() {
        render_groups(batch_groups_, batch_frame_);
        for (texture_batch_group& group : batch_groups_) group.instances.clear();
    }

    // Uploads every group's instances into one contiguous allocation, then issues one instanced
    // draw per group at the matching offset.
    void render_groups(const std::vector<texture_batch_group>& groups, metal_frame* frame) {
        if (!frame) return;
        metal_frame::impl& impl = frame->internal();
        if (!impl.encoder) return;

        size_t instance_count = 0;
        for (const texture_batch_group& group : groups) {
            instance_count += group.instances.size();
        }
        if (instance_count == 0) return;

        const metal_frame::impl::allocation allocation =
            impl.allocate(instance_count * sizeof(glyph_instance_data));
        if (!allocation.pointer) return;

        auto* destination = static_cast<glyph_instance_data*>(allocation.pointer);
        size_t first = 0;
        for (const texture_batch_group& group : groups) {
            if (group.instances.empty()) continue;
            std::memcpy(destination + first, group.instances.data(),
                        group.instances.size() * sizeof(glyph_instance_data));
            first += group.instances.size();
        }

        id<MTLRenderCommandEncoder> encoder = impl.encoder;
        [encoder setVertexBuffer:allocation.buffer offset:allocation.offset atIndex:0];
        first = 0;
        for (const texture_batch_group& group : groups) {
            const size_t count = group.instances.size();
            if (count == 0) continue;
            const atlas_set* atlas = group.key.atlas;
            const int page = group.key.page;
            if (!atlas || page < 0 || static_cast<size_t>(page) >= atlas->active_page_count) {
                first += count;
                continue;
            }

            impl.set_pipeline(device_state().glyph_pipeline(group.key.colored, impl.has_stencil));
            const glyph_vertex_uniforms vertex_uniforms{
                {static_cast<float>(impl.device_size.x), static_cast<float>(impl.device_size.y)},
                static_cast<float>(atlas->size),
                0.0f,
            };
            const glyph_fragment_uniforms fragment_uniforms{group.key.colored ? 1u : 0u,
                                                            group.key.alternate ? 1u : 0u};
            [encoder setVertexBufferOffset:allocation.offset + first * sizeof(glyph_instance_data)
                                   atIndex:0];
            [encoder setVertexBytes:&vertex_uniforms length:sizeof(vertex_uniforms) atIndex:1];
            [encoder setFragmentBytes:&fragment_uniforms
                               length:sizeof(fragment_uniforms)
                              atIndex:0];
            [encoder setFragmentTexture:atlas->pages[static_cast<size_t>(page)].texture atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                        vertexStart:0
                        vertexCount:4
                      instanceCount:count];
            first += count;
        }
    }

    static int atlas_size_for(const px_font_t& font, float scale) {
        const float line_height = font.font->metrics().line_height;
        const unsigned target =
            std::max(1u, static_cast<unsigned>(std::ceil(line_height * scale * 8.0f)));
        return static_cast<int>(std::bit_ceil(target));
    }

    void ensure_phase_pages(atlas_set* atlas,
                            const glyph_atlas_key& requested_key,
                            const fx_glyph_cache::glyph_data& data) {
        glyph_atlas_key first_phase = requested_key;
        first_phase.phase = 0;
        if (atlas->placements.contains(first_phase)) return;

        // A cache miss uploads every subpixel phase at once, so page assignment -- and with it
        // draw order when glyphs overlap -- matches the GL renderer.
        for (size_t phase = 0; phase < fx_glyph_cache::phase_count; ++phase) {
            glyph_atlas_key phase_key = requested_key;
            phase_key.phase = static_cast<int>(phase);
            place(atlas, phase_key, data.phase_at(phase), data.colored);
        }
    }

    static atlas_page* activate_page(atlas_set* atlas) {
        if (atlas->active_page_count < atlas->pages.size()) {
            atlas_page& page = atlas->pages[atlas->active_page_count++];
            page.row_x = 0;
            page.row_y = 0;
            page.row_height = 0;
            return &page;
        }

        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:kColorFormat
                                         width:static_cast<NSUInteger>(atlas->size)
                                        height:static_cast<NSUInteger>(atlas->size)
                                     mipmapped:NO];
        descriptor.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [device_state().device() newTextureWithDescriptor:descriptor];
        if (!texture) {
            return nullptr;
        }
        texture.label = @"px glyph atlas";
        atlas->pages.push_back({.texture = texture});
        ++atlas->active_page_count;
        return &atlas->pages.back();
    }

    static const glyph_atlas_placement* place(atlas_set* atlas,
                                              const glyph_atlas_key& key,
                                              const fx_glyph_cache::glyph_phase& glyph_phase,
                                              bool colored) {
        auto found = atlas->placements.find(key);
        if (found != atlas->placements.end()) {
            return &found->second;
        }

        const int width = static_cast<int>(glyph_phase.width);
        const int height = static_cast<int>(glyph_phase.height);
        if (width <= 0 || height <= 0 || width > atlas->size || height > atlas->size) {
            return nullptr;
        }

        atlas_page* page = atlas->active_page_count == 0
                               ? activate_page(atlas)
                               : &atlas->pages[atlas->active_page_count - 1];
        if (!page) {
            return nullptr;
        }
        if (page->row_x + width > atlas->size) {
            page->row_y += page->row_height;
            page->row_x = 0;
            page->row_height = 0;
        }
        if (page->row_y + height > atlas->size) {
            page = activate_page(atlas);
            if (!page) {
                return nullptr;
            }
        }
        const int atlas_x = page->row_x;
        const int atlas_y = page->row_y;
        const int page_index = static_cast<int>(atlas->active_page_count - 1);

        // The tile is already premultiplied BGRA, the texture's native layout. Only new regions
        // are ever written, so a frame in flight never observes a placement changing under it.
        [page->texture replaceRegion:MTLRegionMake2D(static_cast<NSUInteger>(atlas_x),
                                                     static_cast<NSUInteger>(atlas_y),
                                                     static_cast<NSUInteger>(width),
                                                     static_cast<NSUInteger>(height))
                         mipmapLevel:0
                           withBytes:glyph_phase.pixels
                         bytesPerRow:static_cast<NSUInteger>(width) * 4u];

        glyph_atlas_placement placement{
            .u = static_cast<float>(atlas_x),
            .v = static_cast<float>(atlas_y),
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .page = page_index,
            .colored = colored,
        };
        page->row_x += width;
        page->row_height = std::max(page->row_height, height);
        return &atlas->placements.emplace(key, placement).first->second;
    }

    std::atomic<bool> reset_requested_ = false;
    std::map<std::pair<const px_font_t*, uint32_t>, atlas_set> atlas_sets_;
    int batch_depth_ = 0;
    metal_frame* batch_frame_ = nullptr;
    std::vector<texture_batch_group> batch_groups_;
};

metal_text_render_state& text_render_state() {
    static auto* state = new metal_text_render_state;
    return *state;
}

}  // namespace

class metal_rect_batch {
public:
    explicit metal_rect_batch(metal_frame* frame) : frame_(frame) {}
    ~metal_rect_batch() { flush(); }

    void add(rect area, fcolor color) {
        // ST blends premultiplied colors with ONE, ONE_MINUS_SRC_ALPHA.
        instances_.push_back(rect_instance{static_cast<float>(area.x), static_cast<float>(area.y),
                                           static_cast<float>(area.w), static_cast<float>(area.h),
                                           color.r * color.a, color.g * color.a, color.b * color.a,
                                           color.a});
    }

    void flush() {
        draw_rects(instances_, frame_, false);
        instances_.clear();
    }

    int depth = 1;

private:
    metal_frame* frame_ = nullptr;
    std::vector<rect_instance> instances_;
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// metal_render_context
// ─────────────────────────────────────────────────────────────────────────────────────────────────

rect metal_render_context::normalize_dirty_rects(std::vector<rect>* dirty, rect window_bounds) {
    std::vector<rect> normalized;
    normalized.reserve(dirty->size());
    rect bounds;
    for (rect area : *dirty) {
        area = intersect_rect(area, window_bounds);
        if (area.empty()) continue;
        normalized.push_back(area);
        bounds = union_rect(bounds, area);
    }
    if (normalized.empty()) {
        normalized.push_back(window_bounds);
        bounds = window_bounds;
    }
    if (normalized.size() > kMaximumDirtyRects) {
        normalized.clear();
        normalized.push_back(bounds);
    }
    dirty->swap(normalized);
    return bounds;
}

metal_render_context::metal_render_context(metal_frame* frame,
                                           double dpi_scale,
                                           const rect* dirty,
                                           int dirty_count,
                                           uint32_t subpixel_order)
    : frame_(frame),
      device_size_(frame ? frame->device_size() : vec2{}),
      dpi_scale_(dpi_scale > 0.0 ? dpi_scale : 1.0),
      subpixel_order_(subpixel_order),
      has_stencil_(frame && frame->has_stencil()),
      scale_{dpi_scale_, dpi_scale_} {
    // A batch can only refer to the frame that opened it; anything left over belongs to a frame
    // that no longer exists.
    text_render_state().abandon_batch();

    const rect full_bounds{0.0, 0.0, device_size_.x / dpi_scale_, device_size_.y / dpi_scale_};
    if (!dirty || dirty_count <= 0) {
        dirty = &full_bounds;
        dirty_count = 1;
    }
    for (int i = 0; i < dirty_count; ++i) paint_bounds_ = union_rect(paint_bounds_, dirty[i]);
    if (paint_bounds_.empty()) {
        paint_bounds_ = full_bounds;
        dirty = &full_bounds;
        dirty_count = 1;
    }
    clip_ = device_rect(paint_bounds_);

    if (has_stencil_) {
        establish_dirty_mask(dirty, dirty_count);
    }
    apply_clip();
}

metal_render_context::~metal_render_context() { finish(); }

recti metal_render_context::device_rect(rect area) const {
    const rect transformed = transformed_rect(area);
    const double x0 = transformed.x;
    const double y0 = transformed.y;
    const double x1 = transformed.right();
    const double y1 = transformed.bottom();
    recti result{static_cast<int>(std::floor(std::min(x0, x1))),
                 static_cast<int>(std::floor(std::min(y0, y1))),
                 static_cast<int>(std::ceil(std::max(x0, x1))),
                 static_cast<int>(std::ceil(std::max(y0, y1)))};
    return intersect_recti(
        result, recti{0, 0, static_cast<int>(device_size_.x), static_cast<int>(device_size_.y)});
}

rect metal_render_context::transformed_rect(rect area) const {
    const double x0 = translation_.x + area.x * scale_.x;
    const double y0 = translation_.y + area.y * scale_.y;
    const double x1 = translation_.x + area.right() * scale_.x;
    const double y1 = translation_.y + area.bottom() * scale_.y;
    return rect{std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0), std::abs(y1 - y0)};
}

void metal_render_context::apply_clip() {
    // Every draw path rejects work under an empty clip, so there is no need to express one as a
    // scissor; Metal validates scissor rectangles against the attachment and a degenerate one is
    // not worth the risk.
    if (!frame_ || clip_.empty()) return;
    metal_frame::impl& impl = frame_->internal();
    if (!impl.encoder) return;
    const MTLScissorRect scissor{
        static_cast<NSUInteger>(clip_.left), static_cast<NSUInteger>(clip_.top),
        static_cast<NSUInteger>(clip_.width()), static_cast<NSUInteger>(clip_.height())};
    [impl.encoder setScissorRect:scissor];
}

void metal_render_context::establish_dirty_mask(const rect* dirty, int dirty_count) {
    if (!frame_ || !device_state().usable()) return;
    metal_frame::impl& impl = frame_->internal();
    if (!impl.encoder) return;

    // ST's path: rasterize all dirty rectangles in one instanced draw with color writes disabled
    // and REPLACE stencil operations. The pass cleared the stencil to zero, so afterwards only the
    // exact disjoint dirty region carries the reference value that the test state demands.
    std::vector<rect_instance> instances;
    instances.reserve(static_cast<size_t>(dirty_count));
    for (int i = 0; i < dirty_count; ++i) {
        const rect area = transformed_rect(dirty[i]);
        if (area.empty()) continue;
        instances.push_back(rect_instance{static_cast<float>(area.x), static_cast<float>(area.y),
                                          static_cast<float>(area.w), static_cast<float>(area.h),
                                          0.0f, 0.0f, 0.0f, 0.0f});
    }
    impl.set_depth_stencil(device_state().stencil_write_state());
    draw_rects(instances, frame_, true);
    impl.set_depth_stencil(device_state().stencil_test_state());
}

void metal_render_context::draw_rect(rect area, fill_mode fill) {
    const rect transformed = transformed_rect(area);
    const fcolor normalized = fill.color;
    if (transformed.empty() || normalized.a <= 0.0f || clip_.empty()) return;
    text_render_state().flush_pending_batch();
    if (rect_batch_) {
        rect_batch_->add(transformed, normalized);
        return;
    }
    metal_rect_batch batch(frame_);
    batch.add(transformed, normalized);
}

void metal_render_context::draw_shaped_text(
    px_font_t* font, vec2 position, color value, fx_layout* layout, bool subpixel_positioning) {
    const fcolor normalized = value;
    if (!layout || normalized.a <= 0.0f) {
        return;
    }
    if (rect_batch_) {
        rect_batch_->flush();
    }
    text_render_state().draw(font, *layout, position, normalized, translation_, scale_, frame_,
                             clip_, subpixel_positioning, subpixel_order_);
}

void metal_render_context::translate(double x, double y) {
    translation_.x += x * scale_.x;
    translation_.y += y * scale_.y;
}

void metal_render_context::scale(double x, double y) {
    scale_.x *= x;
    scale_.y *= y;
}

void metal_render_context::restrict_clip_rect(rect area) {
    if (rect_batch_) rect_batch_->flush();
    text_render_state().flush_pending_batch();
    clip_ = intersect_recti(clip_, device_rect(area));
    apply_clip();
}

void metal_render_context::push_state(bool preserve_batch) {
    saved_state state{translation_, scale_, clip_};
    if (!preserve_batch) {
        state.has_batch_state = true;
        state.rect_batch = std::move(rect_batch_);
    }
    state_stack_.push_back(std::move(state));
}

void metal_render_context::pop_state() {
    if (state_stack_.empty()) return;
    // Glyph instances do not carry a clip rectangle. Submit child text while the child scissor is
    // still active, before restoring the parent state below.
    text_render_state().flush_pending_batch();
    saved_state state = std::move(state_stack_.back());
    state_stack_.pop_back();
    if (state.has_batch_state) {
        // Submit child drawing before returning to the parent's batch, preserving draw order.
        rect_batch_.reset();
        rect_batch_ = std::move(state.rect_batch);
    } else if (rect_batch_) {
        // Instances queued under the child clip must be submitted before restoring the parent
        // clip.
        rect_batch_->flush();
    }
    translation_ = state.translation;
    scale_ = state.scale;
    clip_ = state.clip;
    apply_clip();
}

void metal_render_context::begin_rect_batch() {
    text_render_state().flush_pending_batch();
    if (rect_batch_) {
        ++rect_batch_->depth;
        return;
    }
    rect_batch_ = std::make_unique<metal_rect_batch>(frame_);
}

void metal_render_context::begin_text_batch() {
    if (rect_batch_) rect_batch_->flush();
    text_render_state().begin_batch(frame_);
}

void metal_render_context::end_text_batch() { text_render_state().end_batch(); }

void metal_render_context::end_rect_batch() {
    if (!rect_batch_) return;
    if (--rect_batch_->depth == 0) rect_batch_.reset();
}

void metal_render_context::begin_line_batch() {}

void metal_render_context::end_line_batch() {}

void metal_render_context::finish() {
    // pop_state() first submits child work under the child clip and then restores its parent
    // batch. Repeating that operation preserves ordering even for accidentally unbalanced state
    // scopes.
    while (!state_stack_.empty()) pop_state();
    rect_batch_.reset();
    text_render_state().finish_batch();
    if (frame_) frame_->end();
}

void metal_render_context::reset_glyph_atlas_for_testing() { text_render_state().request_reset(); }
