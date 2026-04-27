#include "base/apple/display_link_mac.h"
#include "base/check.h"
#include "base/strings/sys_string_conversions.h"
#include "gfx/device.h"
#include "gl/loader.h"
#include "platform/cocoa/app_mac.h"
#include "platform/cocoa/window_mac.h"
#include <Cocoa/Cocoa.h>
#include <QuartzCore/CAOpenGLLayer.h>
#include <cmath>

@interface PlatformGLLayer : CAOpenGLLayer {
@public
    platform::WindowMac* window;
    std::unique_ptr<gfx::Device> device;
    std::unique_ptr<gfx::Surface> surface;
    bool gl_functions_loaded;
    int last_width_px;
    int last_height_px;
    float last_scale_factor;
}
- (instancetype)initWithWindow:(platform::WindowMac*)window;
@end

@interface PlatformGLView : NSView {
@public
    platform::WindowMac* window;
    std::unique_ptr<base::apple::DisplayLinkMac> display_link;
}
- (instancetype)initWithFrame:(NSRect)frameRect window:(platform::WindowMac*)window;
- (void)setContinuousRedraw:(bool)enabled;
@end

@interface PlatformWindowDelegate : NSObject <NSWindowDelegate> {
@public
    platform::WindowMac* window;
}
- (instancetype)initWithWindow:(platform::WindowMac*)window;
@end

@implementation PlatformGLLayer

- (instancetype)initWithWindow:(platform::WindowMac*)initWindow {
    self = [super init];
    if (!self) return nil;

    window = initWindow;
    self.contentsScale = NSScreen.mainScreen.backingScaleFactor;
    gl_functions_loaded = false;
    last_width_px = -1;
    last_height_px = -1;
    last_scale_factor = 0;
    return self;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    (void)mask;

    CGLPixelFormatObj pixel_format = nullptr;
    GLint pixel_format_count = 0;
    CGLPixelFormatAttribute attrs[] = {
        kCGLPFAOpenGLProfile,      (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
        kCGLPFAColorSize,          (CGLPixelFormatAttribute)24,
        kCGLPFAAlphaSize,          (CGLPixelFormatAttribute)8,
        kCGLPFADepthSize,          (CGLPixelFormatAttribute)24,
        kCGLPFADoubleBuffer,       kCGLPFAAccelerated,
        (CGLPixelFormatAttribute)0};

    if (CGLChoosePixelFormat(attrs, &pixel_format, &pixel_format_count) != kCGLNoError ||
        !pixel_format) {
        return nullptr;
    }
    return pixel_format;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    (void)pixelFormat;
    (void)timeStamp;

    CGLSetCurrentContext(glContext);

    if (!gl_functions_loaded) {
        gl::load_global_function_pointers();
        gl_functions_loaded = true;
    }

    if (!device) {
        device = gfx::create_device(gfx::Backend::kOpenGL);
        CHECK(device);
        surface = device->create_surface(0, 0);
        CHECK(surface);
    }

    CGSize size = self.bounds.size;
    CGFloat scale = self.contentsScale;
    int width_px = (int)llround(size.width * scale);
    int height_px = (int)llround(size.height * scale);
    float scale_factor = static_cast<float>(scale);

    if (width_px != last_width_px || height_px != last_height_px ||
        scale_factor != last_scale_factor) {
        platform::ResizeEvent resize_event{
            .width_px = width_px,
            .height_px = height_px,
            .scale_factor = scale_factor,
        };
        platform::Event event{resize_event};
        window->emit_event(event);
        last_width_px = width_px;
        last_height_px = height_px;
        last_scale_factor = scale_factor;
    }

    surface->resize(width_px, height_px);
    auto frame = surface->begin_frame();
    CHECK(frame);

    platform::FrameInfo frame_info{
        .time_seconds = timeInterval,
        .width_px = width_px,
        .height_px = height_px,
        .scale_factor = scale_factor,
    };
    platform::Event event{platform::DrawEvent{.frame = *frame, .frame_info = frame_info}};
    window->emit_event(event);
    frame->finish();
}

@end

@implementation PlatformGLView

- (instancetype)initWithFrame:(NSRect)frameRect window:(platform::WindowMac*)initWindow {
    self = [super initWithFrame:frameRect];
    if (!self) return nil;

    window = initWindow;
    self.wantsLayer = YES;
    self.layer = [[PlatformGLLayer alloc] initWithWindow:initWindow];

    CGDirectDisplayID display_id = CGMainDisplayID();
    display_link = base::apple::DisplayLinkMac::create_for_display(display_id);
    CHECK(display_link);
    display_link->set_callback([self]() { [self.layer setNeedsDisplay]; });
    display_link->stop();
    return self;
}

- (void)dealloc {
    if (display_link) {
        display_link->set_callback(nullptr);
        display_link->stop();
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self.layer setNeedsDisplay];
}

- (void)scrollWheel:(NSEvent*)event {
    platform::ScrollEvent scroll_event{
        .dx = static_cast<float>(event.scrollingDeltaX),
        .dy = static_cast<float>(event.scrollingDeltaY),
    };
    platform::Event platform_event{scroll_event};
    window->emit_event(platform_event);
}

- (void)setContinuousRedraw:(bool)enabled {
    if (enabled) {
        display_link->start();
    } else {
        display_link->stop();
    }
}

@end

@implementation PlatformWindowDelegate

- (instancetype)initWithWindow:(platform::WindowMac*)initWindow {
    self = [super init];
    if (!self) return nil;

    window = initWindow;
    return self;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;
    return window->should_close();
}

- (void)windowWillClose:(NSNotification*)notification {
    (void)notification;
    platform::WindowMac* closing_window = window;
    closing_window->did_close();
    closing_window->app().window_did_close(closing_window);
}

@end

namespace platform {

std::unique_ptr<WindowMac> WindowMac::create(AppMac& app,
                                             WindowOptions options,
                                             WindowDelegate* delegate) {
    return std::unique_ptr<WindowMac>(new WindowMac(app, std::move(options), delegate));
}

WindowMac::WindowMac(AppMac& app, WindowOptions options, WindowDelegate* delegate)
    : app_(app), delegate_(delegate) {
    NSRect frame = NSMakeRect(0, 0, options.width, options.height);
    NSUInteger style =
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;

    ns_window_ = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:style
                                               backing:NSBackingStoreBuffered
                                                 defer:false];

    window_delegate_ = [[PlatformWindowDelegate alloc] initWithWindow:this];
    ns_window_.delegate = window_delegate_;

    gl_view_ = [[PlatformGLView alloc] initWithFrame:frame window:this];
    ns_window_.contentView = gl_view_;

    set_title(options.title);
    [ns_window_ center];
    [ns_window_ makeKeyAndOrderFront:nil];
    [ns_window_ makeFirstResponder:gl_view_];
}

WindowMac::~WindowMac() {
    [gl_view_ setContinuousRedraw:false];
    ns_window_.delegate = nil;
}

void WindowMac::set_title(std::string_view title) {
    auto ns_title = base::sys_utf8_to_nsstring(title);
    [ns_window_ setTitle:ns_title];
}

void WindowMac::request_redraw() { [gl_view_.layer setNeedsDisplay]; }

void WindowMac::set_continuous_redraw(bool enabled) { [gl_view_ setContinuousRedraw:enabled]; }

bool WindowMac::should_close() {
    bool allow_close = true;
    Event event{CloseRequestEvent{.allow_close = &allow_close}};
    emit_event(event);
    return allow_close;
}

void WindowMac::did_close() {
    Event event{CloseEvent{}};
    emit_event(event);
    ns_window_ = nil;
    gl_view_ = nil;
    window_delegate_ = nil;
}

void WindowMac::emit_event(const Event& event) {
    if (delegate_) {
        delegate_->on_event(*this, event);
    }
}

AppMac& WindowMac::app() { return app_; }

}  // namespace platform
