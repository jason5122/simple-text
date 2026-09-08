// The CAMetalLayer backing.
//
// ST has no Metal path; this one is laid out to match px_gl_layer.mm so the two can be compared
// frame for frame:
//
//   * One process-wide device and command queue, owned by metal_render_context.mm, so pipelines
//     and glyph atlases are shared by every window the way the single CGL context shares them.
//
//   * Rendering goes into a layer-owned persistent BGRA8 texture with an 8-bit stencil, both grown
//     with 30 pixels of slack. Dirty regions update that stable image, then the visible area is
//     blitted into whichever drawable Core Animation hands out. That is the Metal spelling of the
//     persistent FBO plus glBlitFramebuffer. Rendering straight into the drawable, as Zed does,
//     measured no smoother; the persistent texture keeps ST's dirty-rectangle economy.
//
//   * Drawing is driven by setNeedsDisplayInRect: and the display link through an override of
//     -display. Nothing polls.
//
//   * Presentation is tied to the Core Animation transaction (presentsWithTransaction), which is
//     what CAOpenGLLayer's synchronous drawInCGLContext: provided implicitly: the frame that
//     answers a resize lands in the same commit as the new bounds.

#include "px/mac/px_mac_private.h"
#include "px/metal_render_context.h"
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

@interface PXMetalLayer : CAMetalLayer {
    px_window_t* _pxw;
    std::vector<rect> _dirty;
    std::mutex _dirtyMutex;
    id<MTLCommandQueue> _queue;
    id<MTLTexture> _color;
    id<MTLTexture> _stencil;
    NSUInteger _backingWidth;
    NSUInteger _backingHeight;
    bool _stencilUnavailable;
}
- (instancetype)initWithPXW:(px_window_t*)pxw device:(id<MTLDevice>)device;
- (void)addDirtyRect:(rect)r;
@end

@implementation PXMetalLayer

- (instancetype)initWithPXW:(px_window_t*)pxw device:(id<MTLDevice>)device {
    self = [super init];
    if (self) {
        _pxw = pxw;
        _queue = metal_render_command_queue();
        _backingWidth = 0;
        _backingHeight = 0;
        _stencilUnavailable = false;

        self.device = device;
        self.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.maximumDrawableCount = 3;
        // The drawable is only ever a blit destination, which framebufferOnly forbids.
        self.framebufferOnly = NO;
        self.presentsWithTransaction = YES;

        // Seed with a full-size rect so the first frame is a complete repaint.
        _dirty.push_back(rect{0.0, 0.0, px_window_size(pxw).x, px_window_size(pxw).y});
    }
    return self;
}

- (void)addDirtyRect:(rect)r {
    const std::lock_guard lock(_dirtyMutex);
    _dirty.push_back(r);
}

// Grows the persistent textures to cover `width` x `height`. Returns true when storage was
// replaced, meaning every visible pixel has to be reconstructed this frame.
- (bool)ensureBackingWidth:(NSUInteger)width height:(NSUInteger)height {
    if (_color && _backingWidth >= width && _backingHeight >= height) {
        return false;
    }
    _backingWidth = std::max(_backingWidth, width + 30);
    _backingHeight = std::max(_backingHeight, height + 30);

    id<MTLDevice> device = self.device;
    MTLTextureDescriptor* colorDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                           width:_backingWidth
                                                          height:_backingHeight
                                                       mipmapped:NO];
    colorDescriptor.usage = MTLTextureUsageRenderTarget;
    colorDescriptor.storageMode = MTLStorageModePrivate;
    _color = [device newTextureWithDescriptor:colorDescriptor];
    _color.label = @"px backing color";

    _stencil = nil;
    if (!_stencilUnavailable) {
        MTLTextureDescriptor* stencilDescriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatStencil8
                                                               width:_backingWidth
                                                              height:_backingHeight
                                                           mipmapped:NO];
        stencilDescriptor.usage = MTLTextureUsageRenderTarget;
        stencilDescriptor.storageMode = MTLStorageModePrivate;
        _stencil = [device newTextureWithDescriptor:stencilDescriptor];
        if (_stencil) {
            _stencil.label = @"px backing stencil";
        } else {
            _stencilUnavailable = true;
            NSLog(@"px: no stencil texture; honoring dirty rectangles with the coarse scissor "
                  @"only");
        }
    }
    return true;
}

- (void)display {
    if (!_pxw || !_pxw->handler || !_queue) {
        return;
    }

    const double scale = px_window_dpi_scale_factor(_pxw);
    self.contentsScale = scale;

    // With no colorspace set, the window server colour-matches CAMetalLayer content from sRGB to
    // the display, which on a P3 panel shifts every chromatic pixel (colour emoji, syntax
    // colours). CAOpenGLLayer content and the CG software path are both treated as already in the
    // display's space, so tag the drawable with the window's colour space to make the match the
    // identity and keep the three backends pixel-identical. Re-checked every frame because the
    // window's space follows the screen it is on.
    CGColorSpaceRef windowColorSpace = _pxw->window.colorSpace.CGColorSpace;
    if (windowColorSpace && (!self.colorspace || !CFEqual(self.colorspace, windowColorSpace))) {
        self.colorspace = windowColorSpace;
    }

    const vec2 size = px_window_size(_pxw);
    const vec2 device{std::floor(size.x * scale), std::floor(size.y * scale)};
    const NSUInteger width = static_cast<NSUInteger>(device.x);
    const NSUInteger height = static_cast<NSUInteger>(device.y);
    if (width < 1 || height < 1) {
        return;
    }
    self.drawableSize = CGSizeMake(device.x, device.y);
    const rect windowBounds{0.0, 0.0, size.x, size.y};

    std::vector<rect> dirty;
    {
        const std::lock_guard lock(_dirtyMutex);
        dirty.swap(_dirty);
    }
    // AppKit displays the layer synchronously inside a window resize, after windowDidResize: has
    // marked the window dirty but before the run-loop observer has handed those rectangles to the
    // layer. Take them here: this frame draws them, and the later flush then finds nothing and
    // schedules no second display of content already on screen.
    dirty.insert(dirty.end(), _pxw->dirty.begin(), _pxw->dirty.end());
    _pxw->dirty.clear();
    if (dirty.empty()) {
        dirty.push_back(windowBounds);
    }

    _pxw->handler->pre_paint();
    px_mac_dispatch_post_event_callbacks();

    if ([self ensureBackingWidth:width height:height]) {
        dirty.clear();
        dirty.push_back(windowBounds);
    }
    if (!_color) {
        return;
    }

    // The queue serializes each backing-texture update and its following blit. Let
    // CAMetalLayer's three-drawable swap queue provide backpressure instead of waiting for the
    // previous frame to finish on the GPU.
    id<MTLCommandBuffer> commandBuffer = [_queue commandBuffer];
    if (!commandBuffer) {
        return;
    }
    commandBuffer.label = @"px frame";

    metal_render_context::normalize_dirty_rects(&dirty, windowBounds);
    {
        std::unique_ptr<metal_frame> frame =
            metal_frame_begin(commandBuffer, _color, _stencil, device);
        metal_render_context rc(frame.get(), scale, dirty.data(), static_cast<int>(dirty.size()));
        _pxw->handler->paint(&rc, rc.paint_bounds(), dirty.data(),
                             static_cast<int>(dirty.size()));
        rc.finish();
    }

    // Acquired only now, after the scene is recorded, so a drawable shortage never stalls
    // painting.
    id<CAMetalDrawable> drawable = [self nextDrawable];
    if (drawable) {
        id<MTLTexture> destination = drawable.texture;
        id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
        blit.label = @"px present";
        [blit copyFromTexture:_color
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:MTLOriginMake(0, 0, 0)
                   sourceSize:MTLSizeMake(std::min(width, destination.width),
                                          std::min(height, destination.height), 1)
                    toTexture:destination
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:MTLOriginMake(0, 0, 0)];
        [blit endEncoding];
    }

    [commandBuffer commit];
    if (drawable) {
        // presentsWithTransaction requires the buffer to be scheduled before the drawable is
        // presented; only then does the presentation join the current transaction.
        [commandBuffer waitUntilScheduled];
        [drawable present];
    } else {
        // The pool was exhausted. The persistent texture is up to date, so another display will
        // put it on screen.
        [self setNeedsDisplay];
    }

    _pxw->did_first_paint = true;
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────

CALayer* px_mac_make_metal_layer(px_window_t* window) {
    id<MTLDevice> device = metal_render_device();
    if (!device) {
        NSLog(@"px: no Metal device is available; using OpenGL");
        return nil;
    }

    PXMetalLayer* layer = [[PXMetalLayer alloc] initWithPXW:window device:device];
    layer.needsDisplayOnBoundsChange = YES;
    layer.opaque = window->background.a >= 1.0f;
    layer.contentsScale = px_window_dpi_scale_factor(window);
    return layer;
}

void px_mac_metal_layer_add_dirty(CALayer* layer, rect r) {
    if ([layer isKindOfClass:[PXMetalLayer class]]) {
        [static_cast<PXMetalLayer*>(layer) addDirtyRect:r];
    }
}
