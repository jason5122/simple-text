#include "base/apple/display_link_mac.h"
#include "base/rand_util.h"
#include "gfx/device.h"
#include "gl/loader.h"
#include <Cocoa/Cocoa.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>

namespace {
float rand_range(float minv, float maxv) { return minv + (maxv - minv) * base::rand_float(); }

constexpr std::array<gfx::Quad, 100> make_random_quads(float viewport_w, float viewport_h) {
    std::array<gfx::Quad, 100> quads{};

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

std::array<gfx::Quad, 8> make_animation_quads(double t, int viewport_w, int viewport_h) {
    constexpr float kPi = 3.14159265358979323846f;
    std::array<gfx::Quad, 8> quads{};

    const float cx = viewport_w * 0.5f;
    const float cy = viewport_h * 0.5f;
    const float radius = std::min(viewport_w, viewport_h) * 0.22f;
    const float size = 56.0f + 18.0f * std::sin(t * 2.0);

    for (size_t i = 0; i < 6; ++i) {
        const float phase = static_cast<float>(t * 1.8 + i * (kPi / 3.0f));
        const float x = cx + radius * std::cos(phase) - size * 0.5f;
        const float y = cy + radius * std::sin(phase) - size * 0.5f;
        const float hue = static_cast<float>(i) / 6.0f;
        quads[i] = gfx::Quad{
            x,
            y,
            size,
            size,
            0.25f + 0.75f * std::sin(hue * 6.28318f + 0.0f) * std::sin(hue * 6.28318f + 0.0f),
            0.25f + 0.75f * std::sin(hue * 6.28318f + 2.1f) * std::sin(hue * 6.28318f + 2.1f),
            0.25f + 0.75f * std::sin(hue * 6.28318f + 4.2f) * std::sin(hue * 6.28318f + 4.2f),
            1.f};
    }

    const float bar_x = cx - 220.0f * std::cos(t * 2.4f);
    quads[6] = gfx::Quad{bar_x, cy - 260.0f, 18.0f, 520.0f, 0.05f, 0.05f, 0.05f, 1.f};

    const float pulse = 120.0f + 40.0f * std::sin(t * 3.0f);
    quads[7] =
        gfx::Quad{cx - pulse * 0.5f, cy - pulse * 0.5f, pulse, pulse, 1.0f, 0.82f, 0.18f, 1.f};

    return quads;
}
}  // namespace

@interface GLLayer : CAOpenGLLayer
- (instancetype)init;
@end

@implementation GLLayer {
@public
    std::unique_ptr<gfx::Device> device_;
    std::unique_ptr<gfx::Surface> surface_;
    bool gl_functions_loaded_;
    float scroll_y_;
}

- (instancetype)init {
    self = [super init];
    if (!self) return nil;

    self.contentsScale = NSScreen.mainScreen.backingScaleFactor;

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

    // TODO: Can we initialize once somewhere during setup?
    if (!gl_functions_loaded_) {
        gl::load_global_function_pointers();
        gl_functions_loaded_ = true;
    }
    if (!device_) {
        device_ = gfx::create_device(gfx::Backend::kOpenGL);
    }
    if (!surface_) {
        surface_ = device_->create_surface(0, 0);
    }

    CGSize s = self.bounds.size;
    CGFloat scale = self.contentsScale;
    int w = (int)llround(s.width * scale);
    int h = (int)llround(s.height * scale);

    surface_->resize(w, h);

    auto frame = surface_->begin_frame();
    if (!frame) std::abort();

    frame->clear({1.f, 1.f, 1.f, 1.f});

    // frame->draw_quads(kQuads, 0, -scroll_y_);

    const auto animation_quads = make_animation_quads(timeInterval, w, h);
    frame->draw_quads(animation_quads, 0, 0);

    frame->finish();
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

    display_link_->set_callback([self]() { [self.layer setNeedsDisplay]; });
    display_link_->start();

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

    [self.layer setNeedsDisplay];
}

@end

int main() {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

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
        [window setTitle:@"Renderer Experiment"];
        [window makeKeyAndOrderFront:nil];
        [window makeFirstResponder:gl_view];

        [NSApp run];
    }
}
