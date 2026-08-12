#include "base/debug/profiler.h"
#include "experiments/rasterizer/gl_helpers.h"
#include <AppKit/AppKit.h>
#include <OpenGL/gl3.h>
#include <cstdio>
#include <print>

using base::apple::ScopedCGContext;

namespace {

const char* kBlitVS =
#include "experiments/rasterizer/blit_vert.glsl"
    ;
const char* kBlitFS =
#include "experiments/rasterizer/blit_frag.glsl"
    ;

GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "shader compile failed: %s\n", log);
    }
    return shader;
}

GLuint link_program(const char* vs_src, const char* fs_src) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

}  // namespace

// A view that blits one static texture (the finished CG bitmap) 1:1 to the top-left of the
// surface, matching where the NSImageView path places the same image.
@interface RasterGLView : NSOpenGLView {
    const uint8_t* _data;
    int _tex_w;
    int _tex_h;
    GLuint _program;
    GLuint _vao;
    GLuint _tex;
}
@end

@implementation RasterGLView

- (instancetype)initWithFrame:(NSRect)frame data:(const uint8_t*)data w:(int)w h:(int)h {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAOpenGLProfile,
        NSOpenGLProfileVersion4_1Core,  // GLSL 330+
        NSOpenGLPFAColorSize,          32, NSOpenGLPFADoubleBuffer, NSOpenGLPFAAccelerated, 0,
    };
    NSOpenGLPixelFormat* pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    self = [super initWithFrame:frame pixelFormat:pf];
    if (self) {
        _data = data;
        _tex_w = w;
        _tex_h = h;
        self.wantsBestResolutionOpenGLSurface = YES;  // 2x backing on Retina
    }
    return self;
}

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];

    _program = link_program(kBlitVS, kBlitFS);
    glGenVertexArrays(1, &_vao);  // required to be bound even for attributeless draws

    glGenTextures(1, &_tex);
    glBindTexture(GL_TEXTURE_2D, _tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // The CG bitmap is premultiplied-first, host byte order -> BGRA bytes on little-endian; this
    // format/type pair reads it back as RGBA in the shader.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _tex_w, _tex_h, 0, GL_BGRA,
                 GL_UNSIGNED_INT_8_8_8_8_REV, _data);
}

- (void)drawRect:(NSRect)dirtyRect {
    base::Profiler _("draw");

    [[self openGLContext] makeCurrentContext];

    NSRect backing = [self convertRectToBacking:self.bounds];
    const auto fb_w = static_cast<GLsizei>(backing.size.width);
    const auto fb_h = static_cast<GLsizei>(backing.size.height);
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    glUseProgram(_program);
    // The texture occupies the top-left tex_w x tex_h device pixels of the framebuffer, 1:1; its
    // NDC size is 2 * tex / framebuffer. Computing it here keeps the vertex shader divide-free.
    glUniform2f(glGetUniformLocation(_program, "u_extent"),
                2.0f * static_cast<float>(_tex_w) / static_cast<float>(fb_w),
                2.0f * static_cast<float>(_tex_h) / static_cast<float>(fb_h));
    glUniform1i(glGetUniformLocation(_program, "u_tex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _tex);

    glBindVertexArray(_vao);  // no attributes; the shader derives corners from gl_VertexID
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    [[self openGLContext] flushBuffer];
}

@end

void show_window_gl(ScopedCGContext ctx, double scale) {
    const auto* data = static_cast<const uint8_t*>(CGBitmapContextGetData(ctx.get()));
    const int w = static_cast<int>(CGBitmapContextGetWidth(ctx.get()));
    const int h = static_cast<int>(CGBitmapContextGetHeight(ctx.get()));

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Same window geometry as show_window() so the fixed screencapture region lines up.
    NSWindow* window =
        [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1728, 1117)
                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                              NSWindowStyleMaskResizable
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    RasterGLView* view = [[RasterGLView alloc] initWithFrame:NSMakeRect(0, 0, 1728, 1117)
                                                        data:data
                                                           w:w
                                                           h:h];
    window.contentView = view;
    [window center];
    [window makeKeyAndOrderFront:nil];

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@""];
    [app_menu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"]];
    app_item.submenu = app_menu;
    [main_menu addItem:app_item];
    NSApp.mainMenu = main_menu;

    [NSApp activateIgnoringOtherApps:YES];
    [view display];  // force the first GL frame before signaling readiness

    std::puts("WINDOW_READY");
    std::fflush(stdout);

    [NSApp run];
    (void)scale;
}
