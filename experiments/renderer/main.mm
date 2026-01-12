#include "base/apple/display_link_mac.h"
#include "base/rand_util.h"
#include "gfx/device.h"
#include <Cocoa/Cocoa.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>

namespace {

double now_seconds() { return CACurrentMediaTime(); }

float rand_range(float minv, float maxv) { return minv + (maxv - minv) * base::rand_float(); }

// Generates quads in pixel coords, top-left origin.
// `viewport_w/h` define the render target size in pixels.
constexpr std::array<gfx::Quad, 100> make_random_quads(float viewport_w, float viewport_h) {
    std::array<gfx::Quad, 100> quads{};

    // Tweak these to taste.
    const float min_w = 4.0f;
    const float max_w = std::max(8.0f, viewport_w * 0.35f);
    const float min_h = 4.0f;
    const float max_h = std::max(8.0f, viewport_h * 0.35f);

    for (size_t i = 0; i < quads.size(); ++i) {
        float w = rand_range(min_w, max_w);
        float h = rand_range(min_h, max_h);

        // Keep fully on-screen. If you want off-screen stress too, remove clamps.
        float x = rand_range(0.0f, std::max(0.0f, viewport_w - w));
        float y = rand_range(0.0f, std::max(0.0f, viewport_h - h));

        float r = base::rand_float();
        float g = base::rand_float();
        float b = base::rand_float();

        // Slight bias toward visible alpha, but still varies.
        float a = rand_range(0.15f, 1.0f);

        quads[i] = gfx::Quad{x, y, w, h, r, g, b, a};
    }

    return quads;
}

const auto kQuads = make_random_quads(3000, 2000);

}  // namespace

@interface GLLayer : CAOpenGLLayer
- (instancetype)init;
- (void)requestFrameOnce;
@end

@implementation GLLayer {
@public
    std::unique_ptr<gfx::Device> device_;
    float scroll_y_;
    std::atomic<float> scroll_dy_;
    std::atomic<bool> frame_pending_;
    std::atomic<double> keep_running_until_;
}

- (instancetype)init {
    self = [super init];
    if (!self) return nil;

    self.contentsScale = NSScreen.mainScreen.backingScaleFactor;

    device_ = gfx::create_device(gfx::Backend::kOpenGL);
    if (!device_) {
        spdlog::error("Could not create OpenGL backend.");
        std::exit(EXIT_FAILURE);
    }

    scroll_y_ = 0.0f;
    scroll_dy_.store(0.0f, std::memory_order_relaxed);
    frame_pending_.store(false, std::memory_order_relaxed);
    keep_running_until_.store(0.0, std::memory_order_relaxed);

    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatObj pf = NULL;
    GLint npix = 0;

    CGLPixelFormatAttribute attrs[] = {
        kCGLPFAOpenGLProfile,      (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAColorSize,          (CGLPixelFormatAttribute)24,
        kCGLPFAAlphaSize,          (CGLPixelFormatAttribute)8,
        kCGLPFADepthSize,          (CGLPixelFormatAttribute)24,
        kCGLPFADoubleBuffer,       kCGLPFAAccelerated,
        (CGLPixelFormatAttribute)0};

    if (CGLChoosePixelFormat(attrs, &pf, &npix) != kCGLNoError || !pf) return NULL;
    return pf;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    CGLSetCurrentContext(glContext);

    CGSize s = self.bounds.size;
    CGFloat scale = self.contentsScale;
    int w = (int)llround(s.width * scale);
    int h = (int)llround(s.height * scale);

    // TODO: Move surface somewhere more sensible.
    auto surface = device_->create_surface(w, h);
    if (!surface) std::abort();

    auto frame = surface->begin_frame();
    if (!frame) std::abort();

    frame->set_viewport(w, h);
    frame->clear({1.f, 1.f, 1.f, 1.f});

    // Build a few quads in pixel coordinates (top-left origin).
    constexpr std::array<gfx::Quad, 4> quads = {
        gfx::Quad{800.f, 800.f, 40.f, 600.f, 1.f, 0.f, 0.f, 1.f},
        gfx::Quad{1200.f, 800.f, 40.f, 600.f, 0.f, 1.f, 0.f, 0.6f},
        gfx::Quad{1600.f, 800.f, 40.f, 600.f, 0.2f, 0.4f, 1.f, 1.f},
        gfx::Quad{2000.f, 800.f, 40.f, 600.f, 127.f / 255, 127.f / 255, 127.f / 255, 1.f},
    };

    float dy = scroll_dy_.exchange(0.0f, std::memory_order_relaxed);
    scroll_y_ += dy;

    // frame->draw_quads(quads, 0, -scroll_y_);
    frame->draw_quads(kQuads, 0, -scroll_y_);

    frame->present();

    frame_pending_.store(false, std::memory_order_relaxed);
}

- (void)requestFrameOnce {
    bool expected = false;
    if (!frame_pending_.compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
        spdlog::warn("Frame dropped");
        return;
    }
    [self setNeedsDisplay];
}

@end

@interface GLView : NSView
@end

@implementation GLView {
    std::unique_ptr<base::apple::DisplayLinkMac> display_link_;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (!self) return nil;

    self.wantsLayer = YES;
    self.layer = [[GLLayer alloc] init];

    CGDirectDisplayID display_id = CGMainDisplayID();
    display_link_ = base::apple::DisplayLinkMac::create_for_display(display_id);
    if (!display_link_) std::abort();

    display_link_->set_callback([self]() {
        GLLayer* layer = (GLLayer*)self.layer;

        double now = now_seconds();
        double until = layer->keep_running_until_.load(std::memory_order_relaxed);

        if (now <= until) {
            [layer requestFrameOnce];
        } else {
            display_link_->stop();
        }
    });
    display_link_->stop();

    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    GLLayer* layer = (GLLayer*)self.layer;
    [layer requestFrameOnce];
}

- (void)scrollWheel:(NSEvent*)e {
    GLLayer* layer = (GLLayer*)self.layer;

    float dy = (float)e.scrollingDeltaY;
    layer->scroll_dy_.fetch_add(dy, std::memory_order_relaxed);
    layer->keep_running_until_.store(now_seconds() + 0.75, std::memory_order_relaxed);

    display_link_->start();
}

@end

int main() {
    @autoreleasepool {
        [NSApplication sharedApplication];

        NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
        [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"]];
        item.submenu = submenu;
        [main_menu addItem:item];
        NSApp.mainMenu = main_menu;

        NSRect frame = NSMakeRect(0, 0, 1200, 800);
        NSUInteger style =
            NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;

        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:false];

        GLView* gl_view = [[GLView alloc] initWithFrame:frame];
        window.contentView = gl_view;

        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
        [window setTitle:@"Bare macOS App"];
        [window makeKeyAndOrderFront:nil];
        [window makeFirstResponder:gl_view];

        [NSApp run];
    }
}
