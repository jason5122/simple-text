#include "px/mac/px_offscreen_mac.h"

#include "px/metal_render_context.h"
#import <Metal/Metal.h>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

struct metal_offscreen {
    id<MTLCommandQueue> queue;
    id<MTLTexture> color;
    id<MTLTexture> stencil;
    id<MTLBuffer> readback;
    rect bounds;  // logical, matching what a window of this size would report
    double dpi_scale = 1.0;
    int width = 0;  // device
    int height = 0;
    std::vector<uint32_t> pixels;
    bool painted = false;
};

namespace {

// metal_frame_begin loads the target's existing contents rather than clearing them, because a
// window repaints only its dirty regions over the previous frame. A fresh texture has no previous
// frame, so start it from a known state instead of leaving whatever the allocator handed us
// visible wherever a paint does not cover.
void clear_color(id<MTLCommandQueue> queue, id<MTLTexture> color) {
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = color;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

    id<MTLCommandBuffer> command_buffer = [queue commandBuffer];
    id<MTLRenderCommandEncoder> encoder =
        [command_buffer renderCommandEncoderWithDescriptor:pass];
    [encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
}

std::unique_ptr<metal_offscreen> create(double width, double height, double dpi_scale) {
    if (!(width > 0.0) || !(height > 0.0) || !(dpi_scale > 0.0)) {
        return nullptr;
    }
    id<MTLDevice> device = metal_render_device();
    id<MTLCommandQueue> queue = metal_render_command_queue();
    if (!device || !queue) {
        return nullptr;
    }

    auto surface = std::make_unique<metal_offscreen>();
    surface->queue = queue;
    surface->bounds = {.x = 0.0, .y = 0.0, .w = width, .h = height};
    surface->dpi_scale = dpi_scale;
    surface->width = static_cast<int>(std::lround(width * dpi_scale));
    surface->height = static_cast<int>(std::lround(height * dpi_scale));
    if (surface->width <= 0 || surface->height <= 0) {
        return nullptr;
    }

    // The same descriptors px_metal_layer uses for its backing store, so the render target the
    // pipelines see is the one they see on screen.
    MTLTextureDescriptor* color_descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:static_cast<NSUInteger>(surface->width)
                                    height:static_cast<NSUInteger>(surface->height)
                                 mipmapped:NO];
    color_descriptor.usage = MTLTextureUsageRenderTarget;
    color_descriptor.storageMode = MTLStorageModePrivate;
    surface->color = [device newTextureWithDescriptor:color_descriptor];
    if (!surface->color) {
        return nullptr;
    }
    surface->color.label = @"px offscreen color";

    MTLTextureDescriptor* stencil_descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8
                                     width:static_cast<NSUInteger>(surface->width)
                                    height:static_cast<NSUInteger>(surface->height)
                                 mipmapped:NO];
    stencil_descriptor.usage = MTLTextureUsageRenderTarget;
    stencil_descriptor.storageMode = MTLStorageModePrivate;
    surface->stencil = [device newTextureWithDescriptor:stencil_descriptor];
    if (surface->stencil) {
        surface->stencil.label = @"px offscreen stencil";
    }

    // A shared-storage buffer is readable from the CPU on every Mac GPU, which a private render
    // target is not; the blit at the end of each paint copies into it.
    const size_t byte_count = static_cast<size_t>(surface->width) *
                              static_cast<size_t>(surface->height) * sizeof(uint32_t);
    surface->readback = [device newBufferWithLength:byte_count
                                            options:MTLResourceStorageModeShared];
    if (!surface->readback) {
        return nullptr;
    }
    surface->readback.label = @"px offscreen readback";
    surface->pixels.resize(byte_count / sizeof(uint32_t));

    clear_color(queue, surface->color);
    return surface;
}



bool paint_into(metal_offscreen* surface, px_window_event_handler* handler) {
    if (!surface || !handler || !surface->color) {
        return false;
    }
    id<MTLCommandBuffer> command_buffer = [surface->queue commandBuffer];
    if (!command_buffer) {
        return false;
    }
    command_buffer.label = @"px offscreen frame";

    handler->pre_paint();

    // One dirty rectangle covering everything: every capture is a full repaint, so there is no
    // previous frame to preserve and no partial-update ordering to reproduce.
    std::vector<rect> dirty{surface->bounds};
    metal_render_context::normalize_dirty_rects(&dirty, surface->bounds);
    {
        std::unique_ptr<metal_frame> frame =
            metal_frame_begin(command_buffer, surface->color, surface->stencil,
                              {static_cast<double>(surface->width),
                               static_cast<double>(surface->height)});
        if (!frame) {
            return false;
        }
        metal_render_context rc(frame.get(), surface->dpi_scale, dirty.data(),
                                static_cast<int>(dirty.size()));
        handler->paint(&rc, rc.paint_bounds(), dirty.data(), static_cast<int>(dirty.size()));
        rc.finish();
    }

    const size_t row_bytes = static_cast<size_t>(surface->width) * sizeof(uint32_t);
    id<MTLBlitCommandEncoder> blit = [command_buffer blitCommandEncoder];
    blit.label = @"px offscreen readback";
    [blit copyFromTexture:surface->color
                 sourceSlice:0
                 sourceLevel:0
                sourceOrigin:MTLOriginMake(0, 0, 0)
                  sourceSize:MTLSizeMake(static_cast<NSUInteger>(surface->width),
                                         static_cast<NSUInteger>(surface->height), 1)
                    toBuffer:surface->readback
           destinationOffset:0
      destinationBytesPerRow:row_bytes
    destinationBytesPerImage:row_bytes * static_cast<size_t>(surface->height)];
    [blit endEncoding];

    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    if (command_buffer.status != MTLCommandBufferStatusCompleted) {
        return false;
    }

    std::memcpy(surface->pixels.data(), surface->readback.contents,
                surface->pixels.size() * sizeof(uint32_t));
    surface->painted = true;
    return true;
}

}  // namespace

// Adapts the surface above to the shared backend interface.
namespace {
class metal_backend final : public px_mac_offscreen {
public:
    explicit metal_backend(std::unique_ptr<metal_offscreen> surface)
        : surface_(std::move(surface)) {}

    bool paint(px_window_event_handler* handler) override {
        return paint_into(surface_.get(), handler);
    }
    const uint32_t* pixels() const override {
        return surface_->painted ? surface_->pixels.data() : nullptr;
    }
    int width() const override { return surface_->width; }
    int height() const override { return surface_->height; }

private:
    std::unique_ptr<metal_offscreen> surface_;
};
}  // namespace

std::unique_ptr<px_mac_offscreen> px_create_metal_offscreen(double width,
                                                            double height,
                                                            double dpi_scale) {
    std::unique_ptr<metal_offscreen> surface = create(width, height, dpi_scale);
    return surface ? std::make_unique<metal_backend>(std::move(surface)) : nullptr;
}
