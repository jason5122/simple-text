#include "base/apple/display_link_mac.h"
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

}  // namespace

@interface GLLayer : CAOpenGLLayer
- (instancetype)init;
@end

@implementation GLLayer {
@public
    bool did_load_gl_;
    GLuint prog_;
    GLuint vao_;
    GLuint vbo_;
    bool anchor_top_left_;

    float scroll_y_;

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

    const float half_w_px = 20.0f;
    const float half_h_px = 300.0f;
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

    glUniform2f(offLoc_, cx_clip, cy_clip);
    glUniform2f(sclLoc_, sx_clip, sy_clip);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glUseProgram(0);

    glFlush();
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
    if (self) {
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
    }
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
