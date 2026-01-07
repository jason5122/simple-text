#include "gl/gl.h"
#include "gl/loader.h"
#include <Cocoa/Cocoa.h>
#include <OpenGL/OpenGL.h>
#include <QuartzCore/QuartzCore.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>

using namespace gl;

namespace {

double NowSeconds() { return CACurrentMediaTime(); }

GLuint Compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei n = 0;
        glGetShaderInfoLog(s, sizeof(log), &n, log);
        spdlog::error("shader compile fail: {}", log);
    }
    return s;
}

GLuint Link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        GLsizei n = 0;
        glGetProgramInfoLog(p, sizeof(log), &n, log);
        spdlog::error("program link fail: {}", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

inline void AtomicMax(std::atomic<double>& a, double v) {
    double cur = a.load(std::memory_order_relaxed);
    while (v > cur && !a.compare_exchange_weak(cur, v, std::memory_order_relaxed)) {
    }
}

}  // namespace

@interface GLLayer : CAOpenGLLayer
- (instancetype)init;
- (void)requestFrameOnce;
@end

@implementation GLLayer {
@public
    bool did_load_gl_;
    GLuint prog_;
    GLuint vao_;
    GLuint vbo_;
    bool anchor_top_left_;

    float scroll_y_;
    std::atomic<float> scroll_dy_;
    std::atomic<bool> frame_pending_;
    std::atomic<double> keep_running_until_;
    std::atomic<double> nominal_dt_s_;

    std::atomic<uint64_t> draw_count_;

    std::atomic<double> last_draw_s_;
    std::atomic<uint64_t> dt_n_;
    std::atomic<double> dt_sum_;
    std::atomic<double> dt_sum2_;
    std::atomic<double> dt_max_;
    std::atomic<uint64_t> missed_;

    std::atomic<double> last_scroll_event_s_;
    std::atomic<double> last_consumed_input_s_;
    std::atomic<uint64_t> in_n_;
    std::atomic<double> in_sum_;
    std::atomic<double> in_max_;

    GLint offLoc_;
    GLint sclLoc_;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        self.contentsScale = NSScreen.mainScreen.backingScaleFactor;

        did_load_gl_ = false;
        prog_ = 0;
        vao_ = 0;
        vbo_ = 0;
        anchor_top_left_ = true;

        scroll_y_ = 0.0f;
        scroll_dy_.store(0.0f, std::memory_order_relaxed);
        frame_pending_.store(false, std::memory_order_relaxed);
        keep_running_until_.store(0.0, std::memory_order_relaxed);
        nominal_dt_s_.store(1.0 / 60.0, std::memory_order_relaxed);

        draw_count_.store(0, std::memory_order_relaxed);

        last_draw_s_.store(0.0, std::memory_order_relaxed);
        dt_n_.store(0, std::memory_order_relaxed);
        dt_sum_.store(0.0, std::memory_order_relaxed);
        dt_sum2_.store(0.0, std::memory_order_relaxed);
        dt_max_.store(0.0, std::memory_order_relaxed);
        missed_.store(0, std::memory_order_relaxed);

        last_scroll_event_s_.store(0.0, std::memory_order_relaxed);
        last_consumed_input_s_.store(0.0, std::memory_order_relaxed);
        in_n_.store(0, std::memory_order_relaxed);
        in_sum_.store(0.0, std::memory_order_relaxed);
        in_max_.store(0.0, std::memory_order_relaxed);
    }
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

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    CGLContextObj ctx = [super copyCGLContextForPixelFormat:pixelFormat];
    if (!ctx) return NULL;

    CGLContextObj prev = CGLGetCurrentContext();
    CGLSetCurrentContext(ctx);

    if (!did_load_gl_) {
        gl::load_global_function_pointers();
        did_load_gl_ = true;

        const char* vs_src =
#include "experiments/renderer/vert.glsl"
            ;

        const char* fs_src =
#include "experiments/renderer/frag.glsl"
            ;

        prog_ = Link(Compile(GL_VERTEX_SHADER, vs_src), Compile(GL_FRAGMENT_SHADER, fs_src));

        float verts[] = {-1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f};

        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        GLint loc = glGetAttribLocation(prog_, "aPos");
        glEnableVertexAttribArray((GLuint)loc);
        glVertexAttribPointer((GLuint)loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        glBindVertexArray(0);

        offLoc_ = glGetUniformLocation(prog_, "uOffsetClip");
        sclLoc_ = glGetUniformLocation(prog_, "uScaleClip");
    }

    CGLSetCurrentContext(prev);
    return ctx;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    // NOTE: Disable this in case it causes extra lag.
    // spdlog::info("draw");

    double now = NowSeconds();

    double prev = last_draw_s_.exchange(now, std::memory_order_relaxed);
    if (prev > 0.0) {
        double dt = now - prev;
        dt_n_.fetch_add(1, std::memory_order_relaxed);
        dt_sum_.fetch_add(dt, std::memory_order_relaxed);
        dt_sum2_.fetch_add(dt * dt, std::memory_order_relaxed);
        AtomicMax(dt_max_, dt);

        double nominal = nominal_dt_s_.load(std::memory_order_relaxed);
        if (nominal > 0.0 && dt > 1.5 * nominal) missed_.fetch_add(1, std::memory_order_relaxed);
    }

    draw_count_.fetch_add(1, std::memory_order_relaxed);

    float dy = scroll_dy_.exchange(0.0f, std::memory_order_relaxed);
    if (dy != 0.0f) {
        double t_in = last_scroll_event_s_.load(std::memory_order_relaxed);
        double last_counted = last_consumed_input_s_.load(std::memory_order_relaxed);
        if (t_in > 0.0 && t_in != last_counted) {
            last_consumed_input_s_.store(t_in, std::memory_order_relaxed);
            double lat = now - t_in;
            in_n_.fetch_add(1, std::memory_order_relaxed);
            in_sum_.fetch_add(lat, std::memory_order_relaxed);
            AtomicMax(in_max_, lat);
        }
        scroll_y_ += dy;
    }

    CGLSetCurrentContext(glContext);

    CGSize s = self.bounds.size;
    CGFloat scale = self.contentsScale;
    int w = (int)llround(s.width * scale);
    int h = (int)llround(s.height * scale);

    glViewport(0, 0, w, h);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(prog_);
    glBindVertexArray(vao_);

    const float half_w_px = 100.0f;
    const float half_h_px = 100.0f;
    const float margin_px = 100.0f;
    float anim_px = 200.0f;

    float cx_px, cy_px;
    if (anchor_top_left_) {
        cx_px = margin_px + half_w_px + anim_px;
        cy_px = (float)h - (margin_px + half_h_px);
    } else {
        cx_px = (float)w - (margin_px + half_w_px + anim_px);
        cy_px = margin_px + half_h_px;
    }

    cy_px += scroll_y_;

    float cx_clip = (cx_px / (float)w) * 2.0f - 1.0f;
    float cy_clip = (cy_px / (float)h) * 2.0f - 1.0f;

    float sx_clip = (half_w_px / (float)w) * 2.0f;
    float sy_clip = (half_h_px / (float)h) * 2.0f;

    offLoc_ = glGetUniformLocation(prog_, "uOffsetClip");
    sclLoc_ = glGetUniformLocation(prog_, "uScaleClip");
    glUniform2f(offLoc_, cx_clip, cy_clip);
    glUniform2f(sclLoc_, sx_clip, sy_clip);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);

    glFlush();

    frame_pending_.store(false, std::memory_order_release);
}

- (void)requestFrameOnce {
    bool expected = false;
    if (!frame_pending_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    [self setNeedsDisplay];
}

@end

@interface GLView : NSView
@end

namespace {

// CVDisplayLink callback thread -> hop to main.
CVReturn CVDisplayLinkCB(CVDisplayLinkRef,
                         const CVTimeStamp*,
                         const CVTimeStamp*,
                         CVOptionFlags,
                         CVOptionFlags*,
                         void* ctx) {
    GLView* view = (__bridge GLView*)ctx;
    dispatch_async(dispatch_get_main_queue(), ^{
      // call an ObjC method on main
      [view performSelector:@selector(onCVDisplayLinkTick) withObject:nil];
    });
    return kCVReturnSuccess;
}

}  // namespace

@implementation GLView {
    // Use either CADisplayLink (macOS 14+) or CVDisplayLink (older).
    CADisplayLink* ca_dl_ API_AVAILABLE(macos(14.0));  // nil on < macOS 14
    CVDisplayLinkRef cv_dl_;                           // NULL on macOS 14+

    std::atomic<uint64_t> tick_count_;

    double last_stats_s_;
    uint64_t last_ticks_;
    uint64_t last_draws_;

    // For estimating nominal dt with CVDisplayLink.
    double last_cv_tick_s_;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        self.wantsLayer = YES;
        self.layer = [[GLLayer alloc] init];

        tick_count_.store(0, std::memory_order_relaxed);
        last_stats_s_ = 0.0;
        last_ticks_ = 0;
        last_draws_ = 0;
        last_cv_tick_s_ = 0.0;

        ca_dl_ = nil;
        cv_dl_ = NULL;

        if (@available(macOS 14.0, *)) {
            // NSView creates a CADisplayLink that tracks the view’s display automatically.
            ca_dl_ = [self displayLinkWithTarget:self selector:@selector(onCADisplayLink:)];
            [ca_dl_ addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
            ca_dl_.paused = YES;
        } else {
            CVDisplayLinkCreateWithActiveCGDisplays(&cv_dl_);
            CVDisplayLinkSetOutputCallback(cv_dl_, &CVDisplayLinkCB, (__bridge void*)self);
            // Start stopped. We'll start when needed.
        }
    }
    return self;
}

- (void)dealloc {
    if (@available(macOS 14.0, *)) {
        if (ca_dl_) {
            ca_dl_.paused = YES;
            [ca_dl_ invalidate];
            ca_dl_ = nil;
        }
    }
    if (cv_dl_) {
        CVDisplayLinkStop(cv_dl_);
        CVDisplayLinkRelease(cv_dl_);
        cv_dl_ = NULL;
    }
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)startPacer {
    if (@available(macOS 14.0, *)) {
        if (ca_dl_ && ca_dl_.paused) ca_dl_.paused = NO;
    } else {
        if (cv_dl_ && !CVDisplayLinkIsRunning(cv_dl_)) {
            last_cv_tick_s_ = 0.0;
            CVDisplayLinkStart(cv_dl_);
        }
    }
}

- (void)stopPacerIfIdle {
    if (@available(macOS 14.0, *)) {
        if (ca_dl_ && !ca_dl_.paused) ca_dl_.paused = YES;
    } else {
        if (cv_dl_ && CVDisplayLinkIsRunning(cv_dl_)) CVDisplayLinkStop(cv_dl_);
    }
}

// Live resize: you said this is required for zero judder.
// Keep it. This is independent of display-link pacing.
- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    GLLayer* layer = (GLLayer*)self.layer;
    [layer requestFrameOnce];
}

- (void)tickCommonWithNominal:(double)nominal_dt {
    tick_count_.fetch_add(1, std::memory_order_relaxed);

    GLLayer* layer = (GLLayer*)self.layer;

    if (nominal_dt > 0.0 && nominal_dt < 0.5) {
        layer->nominal_dt_s_.store(nominal_dt, std::memory_order_relaxed);
    }

    double now = NowSeconds();
    double until = layer->keep_running_until_.load(std::memory_order_relaxed);

    if (now <= until) {
        [layer requestFrameOnce];
    } else {
        [self stopPacerIfIdle];
    }

    if (last_stats_s_ == 0.0) last_stats_s_ = now;
    if (now - last_stats_s_ >= 1.0) {
        uint64_t ticks = tick_count_.load(std::memory_order_relaxed);
        uint64_t draws = layer->draw_count_.load(std::memory_order_relaxed);

        uint64_t n = layer->dt_n_.exchange(0, std::memory_order_relaxed);
        double sum = layer->dt_sum_.exchange(0.0, std::memory_order_relaxed);
        double sum2 = layer->dt_sum2_.exchange(0.0, std::memory_order_relaxed);
        double mx = layer->dt_max_.exchange(0.0, std::memory_order_relaxed);
        uint64_t missed = layer->missed_.exchange(0, std::memory_order_relaxed);

        double mean_ms = 0.0, sd_ms = 0.0;
        if (n > 0) {
            double mean = sum / (double)n;
            double var = (sum2 / (double)n) - mean * mean;
            if (var < 0.0) var = 0.0;
            mean_ms = mean * 1000.0;
            sd_ms = std::sqrt(var) * 1000.0;
        }

        uint64_t in_n = layer->in_n_.exchange(0, std::memory_order_relaxed);
        double in_sum = layer->in_sum_.exchange(0.0, std::memory_order_relaxed);
        double in_max = layer->in_max_.exchange(0.0, std::memory_order_relaxed);
        double in_mean_ms = (in_n > 0) ? (in_sum / (double)in_n) * 1000.0 : 0.0;
        double in_max_ms = in_max * 1000.0;

        // spdlog::info(
        //     "ticks/s={} draws/s={} mean_dt_ms={:.3f} sd_ms={:.3f} max_dt_ms={:.3f} missed={} "
        //     "input_mean_ms={:.3f} input_max_ms={:.3f} input_samples={}",
        //     (ticks - last_ticks_), (draws - last_draws_), mean_ms, sd_ms, mx * 1000.0, missed,
        //     in_mean_ms, in_max_ms, (unsigned long long)in_n);

        last_ticks_ = ticks;
        last_draws_ = draws;
        last_stats_s_ = now;
    }
}

- (void)onCADisplayLink:(CADisplayLink*)link API_AVAILABLE(macos(14.0)) {
    // nominal dt = targetTimestamp - timestamp
    double nominal = link.targetTimestamp - link.timestamp;
    [self tickCommonWithNominal:nominal];
}

- (void)onCVDisplayLinkTick {
    // CVDisplayLink doesn't give an ObjC object; we estimate dt using callback spacing on main.
    double now = NowSeconds();
    double nominal = 0.0;
    if (last_cv_tick_s_ > 0.0) nominal = now - last_cv_tick_s_;
    last_cv_tick_s_ = now;

    [self tickCommonWithNominal:nominal];
}

- (void)scrollWheel:(NSEvent*)e {
    GLLayer* layer = (GLLayer*)self.layer;

    layer->last_scroll_event_s_.store(NowSeconds(), std::memory_order_relaxed);
    layer->scroll_dy_.fetch_add((float)e.scrollingDeltaY, std::memory_order_relaxed);

    layer->keep_running_until_.store(NowSeconds() + 0.75, std::memory_order_relaxed);
    [self startPacer];
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
