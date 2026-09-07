// An NSWindow with nothing of ours in it, plus the smallest possible GL and Metal layers, so the
// resize behavior of AppKit, Core Animation and the two GPU APIs can be judged with no px code in
// the loop. Drag the corner next to empty_scene, which is px doing the same nothing.
//
//   ./out/release/empty_window            plain content view; AppKit paints the background
//   ./out/release/empty_window --view     a custom NSView filling itself in drawRect: (Core Graphics)
//   ./out/release/empty_window --gl       a CAOpenGLLayer that only clears (simple_text's setup)
//   ./out/release/empty_window --metal    a CAMetalLayer that only clears, presenting with the
//                                         transaction the way Zed does during a resize
//
// The layer modes are layer-hosting (`view.layer = layer`, as simple_text and the startup
// benchmark do). px's own layer-backed setup is what empty_scene exercises.

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#import <QuartzCore/QuartzCore.h>
#include <cmath>
#include <cstring>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {
constexpr float kGrey = 0.97f;
}

// ── a plain NSView that draws with Core Graphics ────────────────────────────────────────────────

@interface FillView : NSView
@end

@implementation FillView
- (BOOL)isOpaque {
    return YES;
}
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor colorWithWhite:kGrey alpha:1.0] setFill];
    NSRectFill(self.bounds);
}
@end

// ── the smallest CAOpenGLLayer ─────────────────────────────────────────────────────────────────

@interface BareGLLayer : CAOpenGLLayer {
    CGLPixelFormatObj _pixelFormat;
    CGLContextObj _context;
}
@end

@implementation BareGLLayer
- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    if (!_pixelFormat) {
        // simple_text's list: profile and offline renderers only.
        const CGLPixelFormatAttribute attributes[] = {
            kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
            kCGLPFAAllowOfflineRenderers, static_cast<CGLPixelFormatAttribute>(0)};
        GLint count = 0;
        CGLChoosePixelFormat(attributes, &_pixelFormat, &count);
    }
    return _pixelFormat;
}
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}
- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    if (!_context) {
        CGLCreateContext(pixelFormat, nullptr, &_context);
    }
    return _context;
}
- (void)releaseCGLContext:(CGLContextObj)context {
}
- (void)drawInCGLContext:(CGLContextObj)context
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)t
             displayTime:(const CVTimeStamp*)ts {
    glClearColor(kGrey, kGrey, kGrey, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    [super drawInCGLContext:context pixelFormat:pixelFormat forLayerTime:t displayTime:ts];
}
@end

// ── the smallest CAMetalLayer ──────────────────────────────────────────────────────────────────

@interface BareMetalLayer : CAMetalLayer {
    id<MTLCommandQueue> _queue;
}
@end

@implementation BareMetalLayer
- (instancetype)init {
    self = [super init];
    if (self) {
        self.device = MTLCreateSystemDefaultDevice();
        self.pixelFormat = MTLPixelFormatBGRA8Unorm;
        self.presentsWithTransaction = YES;
        _queue = [self.device newCommandQueue];
    }
    return self;
}
- (void)display {
    const CGSize bounds = self.bounds.size;
    self.drawableSize = CGSizeMake(std::floor(bounds.width * self.contentsScale),
                                   std::floor(bounds.height * self.contentsScale));
    id<CAMetalDrawable> drawable = [self nextDrawable];
    if (!drawable) {
        return;
    }
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(kGrey, kGrey, kGrey, 1.0);
    id<MTLCommandBuffer> commandBuffer = [_queue commandBuffer];
    [[commandBuffer renderCommandEncoderWithDescriptor:pass] endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilScheduled];
    [drawable present];
}
@end

// ── a view that hosts one of the layers ────────────────────────────────────────────────────────

@interface LayerHostView : NSView
- (instancetype)initWithLayer:(CALayer*)layer;
@end

@implementation LayerHostView
- (instancetype)initWithLayer:(CALayer*)layer {
    self = [super initWithFrame:NSMakeRect(0, 0, 1, 1)];
    if (self) {
        // Layer-hosting: AppKit only keeps the layer's frame in step with the view.
        layer.needsDisplayOnBoundsChange = YES;
        layer.contentsScale = NSScreen.mainScreen.backingScaleFactor;
        layer.opaque = YES;
        layer.contentsGravity = kCAGravityTopLeft;
        self.layer = layer;
    }
    return self;
}
- (BOOL)isOpaque {
    return YES;
}
@end

@interface EmptyWindowDelegate : NSObject <NSApplicationDelegate>
@end

@implementation EmptyWindowDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}
@end

int main(int argc, char** argv) {
    enum class mode { plain, view, gl, metal } selected = mode::plain;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--view")) {
            selected = mode::view;
        } else if (!std::strcmp(argv[i], "--gl")) {
            selected = mode::gl;
        } else if (!std::strcmp(argv[i], "--metal")) {
            selected = mode::metal;
        }
    }

    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        EmptyWindowDelegate* delegate = [[EmptyWindowDelegate alloc] init];
        NSApp.delegate = delegate;

        const NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                       NSWindowStyleMaskResizable |
                                       NSWindowStyleMaskMiniaturizable;
        NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1200, 800)
                                                       styleMask:mask
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        window.releasedWhenClosed = NO;
        window.backgroundColor = [NSColor colorWithWhite:kGrey alpha:1.0];
        switch (selected) {
        case mode::plain:
            window.title = @"empty window";
            break;
        case mode::view:
            window.title = @"empty window (NSView drawRect:)";
            window.contentView = [[FillView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
            break;
        case mode::gl:
            window.title = @"empty window (CAOpenGLLayer)";
            window.contentView = [[LayerHostView alloc] initWithLayer:[[BareGLLayer alloc] init]];
            break;
        case mode::metal:
            window.title = @"empty window (CAMetalLayer)";
            window.contentView =
                [[LayerHostView alloc] initWithLayer:[[BareMetalLayer alloc] init]];
            break;
        }
        [window center];
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}
