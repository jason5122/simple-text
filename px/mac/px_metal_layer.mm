#include "px/mac/px_mac_private.h"
#include "px/metal_render_context.h"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
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
    NSUInteger _presentedWidth;
    NSUInteger _presentedHeight;
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
        _presentedWidth = 0;
        _presentedHeight = 0;

        self.device = device;
        self.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.maximumDrawableCount = 3;
        // The drawable is only ever a blit destination, which framebufferOnly forbids.
        self.framebufferOnly = NO;
        self.presentsWithTransaction = NO;

        // Seed with a full-size rect so the first frame is a complete repaint.
        _dirty.push_back(rect{0.0, 0.0, px_window_size(pxw).x, px_window_size(pxw).y});
    }
    return self;
}

- (void)addDirtyRect:(rect)r {
    std::lock_guard lock(_dirtyMutex);
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

    double scale = px_window_dpi_scale_factor(_pxw);
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

    vec2 size = px_window_size(_pxw);
    vec2 device{std::floor(size.x * scale), std::floor(size.y * scale)};
    NSUInteger width = static_cast<NSUInteger>(device.x);
    NSUInteger height = static_cast<NSUInteger>(device.y);
    if (width < 1 || height < 1) {
        return;
    }
    self.drawableSize = CGSizeMake(device.x, device.y);
    rect windowBounds{0.0, 0.0, size.x, size.y};

    std::vector<rect> dirty;
    {
        std::lock_guard lock(_dirtyMutex);
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

    // The queue serializes each backing-texture update and its following blit. Let CAMetalLayer's
    // three-drawable swap queue provide backpressure instead of waiting on the GPU ourselves.
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
        _pxw->current_frame_id = ++_pxw->next_frame_id;
        _pxw->handler->paint(&rc, rc.paint_bounds(), dirty.data(), static_cast<int>(dirty.size()));
        _pxw->current_frame_id = 0;
        rc.finish();
    }

    // Acquire only after recording the persistent scene so a temporary drawable shortage never
    // delays painting.
    bool resized = width != _presentedWidth || height != _presentedHeight;
    id<CAMetalDrawable> drawable = [self nextDrawable];
    uint64_t frameId = _pxw->next_frame_id;
    std::function<void(uint64_t, double)> presentedCallback = _pxw->frame_presented_callback;
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
    if (drawable && presentedCallback) {
        [drawable addPresentedHandler:^(id<MTLDrawable> presentedDrawable) {
          double presentedTime = presentedDrawable.presentedTime;
          dispatch_async(dispatch_get_main_queue(), ^{
            presentedCallback(frameId, presentedTime);
          });
        }];
    }

    bool fullScreen = (_pxw->window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    BOOL wantOpaque = fullScreen ? NO : (_pxw->background.a >= 1.0f ? YES : NO);
    if (self.opaque != wantOpaque) {
        self.opaque = wantOpaque;
    }

    if (drawable) {
        self.presentsWithTransaction = resized ? YES : NO;
    }
    if (drawable && !resized) {
        [commandBuffer presentDrawable:drawable];
    }
    [commandBuffer commit];
    if (drawable && resized) {
        // presentsWithTransaction requires the buffer to be scheduled before the drawable is
        // presented; only then does the presentation join the current transaction.
        [commandBuffer waitUntilScheduled];
        [drawable present];
    }
    if (drawable) {
        _presentedWidth = width;
        _presentedHeight = height;
    } else {
        // The pool was exhausted. The persistent texture is up to date, so another display will
        // put it on screen.
        [self setNeedsDisplay];
        if (presentedCallback) {
            presentedCallback(frameId, 0.0);
        }
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
