#include "app/cocoa/gl_view.h"
#include "base/apple/display_link_mac.h"
#include "gfx/device.h"
#include <Cocoa/Cocoa.h>
#include <cstdlib>
#include <memory>

@interface GLLayer : CAOpenGLLayer {
@public
    std::unique_ptr<gfx::Device> device_;
    std::unique_ptr<gfx::Surface> surface_;
    float scroll_y_;
}

- (instancetype)init;

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
    float dy = static_cast<float>(e.scrollingDeltaY);
    layer->scroll_y_ += dy;

    display_link_->start();

    // Push the stop 1 sec into the future (debounce).
    int64_t delay_ns = (int64_t)(1.0 * NSEC_PER_SEC);
    dispatch_time_t deadline = dispatch_time(DISPATCH_TIME_NOW, delay_ns);
    dispatch_source_set_timer(stop_timer_, deadline, DISPATCH_TIME_FOREVER, 10 * NSEC_PER_MSEC);

    [self.layer setNeedsDisplay];
}

@end

@implementation GLLayer
- (instancetype)init {
    self = [super init];
    if (!self) return nil;

    self.contentsScale = NSScreen.mainScreen.backingScaleFactor;

    scroll_y_ = 0.0f;

    device_ = gfx::create_device(gfx::Backend::kOpenGL);
    if (!device_) std::abort();

    surface_ = device_->create_surface(0, 0);
    if (!surface_) std::abort();

    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatObj pf = nullptr;
    GLint npix = 0;

    CGLPixelFormatAttribute attrs[] = {
        kCGLPFAOpenGLProfile,      (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAColorSize,          (CGLPixelFormatAttribute)24,
        kCGLPFAAlphaSize,          (CGLPixelFormatAttribute)8,
        kCGLPFADepthSize,          (CGLPixelFormatAttribute)24,
        kCGLPFADoubleBuffer,       kCGLPFAAccelerated,
        (CGLPixelFormatAttribute)0};

    if (CGLChoosePixelFormat(attrs, &pf, &npix) != kCGLNoError || !pf) return nullptr;
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

    surface_->resize(w, h);

    auto frame = surface_->begin_frame();
    if (!frame) std::abort();

    frame->clear({1.f, 1.f, 1.f, 1.f});

    // Build a few quads in pixel coordinates (top-left origin).
    constexpr std::array<gfx::Quad, 4> quads = {
        gfx::Quad{800.f, 800.f, 40.f, 600.f, 1.f, 0.f, 0.f, 1.f},
        gfx::Quad{1200.f, 800.f, 40.f, 600.f, 0.f, 1.f, 0.f, 0.6f},
        gfx::Quad{1600.f, 800.f, 40.f, 600.f, 0.2f, 0.4f, 1.f, 1.f},
        gfx::Quad{2000.f, 800.f, 40.f, 600.f, 127.f / 255, 127.f / 255, 127.f / 255, 1.f},
    };
    frame->draw_quads(quads, 0, -scroll_y_);
    // frame->draw_quads(kQuads, 0, -scroll_y_);

    frame->finish();
}

@end
