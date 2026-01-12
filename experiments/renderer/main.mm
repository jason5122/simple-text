#include "base/apple/display_link_mac.h"
#include "gfx/device.h"
#include <Cocoa/Cocoa.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>

@interface GLLayer : CAOpenGLLayer
- (instancetype)init;
@end

@implementation GLLayer {
@public
    std::unique_ptr<gfx::Device> device_;
    float scroll_y_;
}

- (instancetype)init {
    self = [super init];
    if (!self) return nil;

    self.contentsScale = NSScreen.mainScreen.backingScaleFactor;

    scroll_y_ = 0.0f;

    device_ = gfx::create_device(gfx::Backend::kOpenGL);
    if (!device_) std::abort();

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
    std::array<gfx::Quad, 4> quads = {
        gfx::Quad{20.f - scroll_y_, 20.f, 200.f, 80.f, 1.f, 0.f, 0.f, 1.f},
        gfx::Quad{60.f, 60.f - scroll_y_, 240.f, 120.f, 0.f, 1.f, 0.f, 0.6f},
        gfx::Quad{320.f - scroll_y_, 40.f, 140.f, 200.f, 0.2f, 0.4f, 1.f, 1.f},
        gfx::Quad{100.f, 100.f - scroll_y_, 20.f, 300.f, 127.f / 255, 127.f / 255, 127.f / 255,
                  1.f},
    };
    frame->draw_quads(quads);

    frame->present();
}

@end

@interface GLView : NSView
@end

@implementation GLView {
    std::unique_ptr<base::apple::DisplayLinkMac> display_link_;
    dispatch_source_t stop_timer_;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (!self) return nil;

    self.wantsLayer = YES;
    self.layer = [[GLLayer alloc] init];

    CGDirectDisplayID display_id = CGMainDisplayID();
    display_link_ = base::apple::DisplayLinkMac::create_for_display(display_id);
    if (!display_link_) std::abort();

    // NOTE: Logging seems to add hiccups. Don't log in the callback.
    display_link_->set_callback([self]() { [self.layer setNeedsDisplay]; });
    display_link_->stop();

    stop_timer_ =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    __weak GLView* weak_self = self;
    dispatch_source_set_event_handler(stop_timer_, ^{
      GLView* self = weak_self;
      if (!self) return;
      self->display_link_->stop();
    });
    dispatch_resume(stop_timer_);

    return self;
}

- (void)dealloc {
    display_link_->set_callback(nullptr);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self.layer setNeedsDisplay];
}

- (void)scrollWheel:(NSEvent*)e {
    GLLayer* layer = (GLLayer*)self.layer;
    layer->scroll_y_ += (float)e.scrollingDeltaY;

    display_link_->start();

    // Push the stop 1 sec into the future (debounce).
    int64_t delay_ns = (int64_t)(1.0 * NSEC_PER_SEC);
    dispatch_time_t deadline = dispatch_time(DISPATCH_TIME_NOW, delay_ns);
    dispatch_source_set_timer(stop_timer_, deadline, DISPATCH_TIME_FOREVER, 10 * NSEC_PER_MSEC);
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
