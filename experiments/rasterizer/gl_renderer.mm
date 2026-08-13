#include "base/debug/profiler.h"
#include "experiments/rasterizer/atlas.h"
#include "experiments/rasterizer/gl_helpers.h"
#include <AppKit/AppKit.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <QuartzCore/QuartzCore.h>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <vector>

namespace {

const char* kGlyphVS =
#include "experiments/rasterizer/glyph_vert.glsl"
    ;
const char* kGlyphFS =
#include "experiments/rasterizer/glyph_frag.glsl"
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

// One instance per glyph (and one for the atlas debug quad): a device-pixel rect, an atlas uv
// rect, a tint color, and a flag telling the fragment shader how to draw it. The four corners come
// from gl_VertexID, so there is no per-vertex data.
struct InstanceData {
    float x, y, w, h;    // rect in device pixels, top-left origin
    float u, v, uw, vh;  // atlas uv rect
    float r, g, b, a;    // tint for mono glyphs; ignored otherwise
    float flags;         // see kFlag* below
};
static_assert(sizeof(InstanceData) == 13 * sizeof(float));

// Per-instance flag values; must match glyph_frag.glsl.
constexpr float kFlagMono = 0.0f;
constexpr float kFlagColor = 1.0f;
constexpr float kFlagDebug = 2.0f;

// Bind the four instanced attributes (divisor 1) of the InstanceData currently in GL_ARRAY_BUFFER.
void set_instance_attribs() {
    constexpr GLsizei stride = sizeof(InstanceData);
    for (GLuint loc = 0; loc < 4; ++loc) {
        const GLint size = loc < 3 ? 4 : 1;  // rect, uv, color are vec4; flags is float
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(sizeof(float) * 4 * loc));
        glVertexAttribDivisor(loc, 1);
    }
}

// 4x4 matrices, column-major to match glUniformMatrix4fv(transpose = GL_FALSE). Each literal below
// is written one column per line -- so it reads as the visual transpose of the math matrix, with
// the translation/last column on the bottom line. The clang-format fences keep that
// hand-alignment.
using Mat4 = std::array<float, 16>;

// Standard matrix product a * b.
Mat4 mul(const Mat4& a, const Mat4& b) {
    Mat4 m{};
    for (size_t col = 0; col < 4; ++col) {
        for (size_t row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (size_t k = 0; k < 4; ++k) sum += a[k * 4 + row] * b[col * 4 + k];
            m[col * 4 + row] = sum;
        }
    }
    return m;
}

Mat4 translate(float x, float y, float z) {
    // clang-format off
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x,    y,    z,    1.0f,
    };
    // clang-format on
}

Mat4 scale(float x, float y, float z) {
    // clang-format off
    return {
        x,    0.0f, 0.0f, 0.0f,
        0.0f, y,    0.0f, 0.0f,
        0.0f, 0.0f, z,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    // clang-format on
}

Mat4 rotate_x(float a) {
    const float c = std::cos(a), s = std::sin(a);
    // clang-format off
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    s,    0.0f,
        0.0f, -s,   c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    // clang-format on
}

Mat4 rotate_y(float a) {
    const float c = std::cos(a), s = std::sin(a);
    // clang-format off
    return {
        c,    0.0f, -s,   0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s,    0.0f, c,    0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    // clang-format on
}

// Perspective projection: fovy in radians, looking down -Z.
Mat4 perspective(float fovy, float aspect, float near, float far) {
    const float f = 1.0f / std::tan(fovy * 0.5f);
    const float nf = near - far;
    // clang-format off
    return {
        f / aspect, 0.0f, 0.0f,                     0.0f,
        0.0f,       f,    0.0f,                     0.0f,
        0.0f,       0.0f, (far + near) / nf,       -1.0f,
        0.0f,       0.0f, (2.0f * far * near) / nf, 0.0f,
    };
    // clang-format on
}

// Orthographic projection mapping the box (left,right)x(bottom,top)x(near,far) onto the NDC cube.
// Passing top < bottom (e.g. bottom = height, top = 0) yields a y-down, top-left-origin pixel
// space.
Mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
    const float rl = right - left;
    const float tb = top - bottom;
    const float fn = far - near;
    // clang-format off
    return {
        2.0f / rl,            0.0f,                 0.0f,                0.0f,
        0.0f,                 2.0f / tb,            0.0f,                0.0f,
        0.0f,                 0.0f,                 -2.0f / fn,          0.0f,
        -(right + left) / rl, -(top + bottom) / tb, -(far + near) / fn, 1.0f,
    };
    // clang-format on
}

}  // namespace

// Draws the shaped text as one textured quad per glyph, sampled from a glyph atlas, plus the whole
// atlas in the upper-right corner for debugging. Everything static (atlas + glyph vertex buffer)
// is built once on the first draw, when a GL context first exists.
@interface AtlasGLView : CAOpenGLLayer {
    const GlyphAtlasSource* _source;  // borrowed; outlives the layer
    double _scroll_x;                 // content scrolled left past the left edge, in device pixels
    double _scroll_y;                 // content scrolled up past the top edge, in device pixels
    double _yaw;                      // text tilt about the vertical axis, in radians
    double _pitch;                    // text tilt about the horizontal axis, in radians (unbound)

    bool _ready;
    GLuint _program;
    GLint _u_proj;
    GLint _u_tex;

    std::unique_ptr<Atlas> _atlas;

    GLuint _glyph_vao;
    GLuint _glyph_vbo;  // per-glyph InstanceData
    GLsizei _glyph_instance_count;

    GLuint _debug_vao;
    GLuint _debug_vbo;  // one InstanceData for the atlas debug quad, refreshed per frame
}
- (instancetype)initWithSource:(const GlyphAtlasSource*)source;
- (void)scrollByPoints:(double)dy;   // dy in view points; positive scrolls content down
- (void)scrollXByPoints:(double)dx;  // dx in view points; positive scrolls content right
- (void)yawByPoints:(double)dx;      // dx in view points; tilts the text about the vertical axis
- (void)pitchByPoints:(double)dy;    // dy in view points; tilts the text about the horizontal axis
@end

@implementation AtlasGLView

- (instancetype)initWithSource:(const GlyphAtlasSource*)source {
    if (self = [super init]) {
        _source = source;
    }
    return self;
}

- (void)scrollByPoints:(double)dy {
    // Points -> device pixels so the content tracks the gesture 1:1. Clamped at the top; the
    // bottom is left open (a real editor would clamp to content height).
    _scroll_y -= dy * self.contentsScale;
    [self setNeedsDisplay];
}

- (void)scrollXByPoints:(double)dx {
    // Horizontal counterpart of scrollByPoints:. Unclamped on both sides (a real editor would
    // clamp to the widest line).
    _scroll_x -= dx * self.contentsScale;
    [self setNeedsDisplay];
}

- (void)yawByPoints:(double)dx {
    // Just for fun: horizontal scroll spins the text about its vertical axis. Clamped short of
    // edge-on (+-90 deg) so the page never disappears or flips to its back.
    constexpr double kRadPerPoint = 0.004;
    _yaw += dx * kRadPerPoint;
    [self setNeedsDisplay];
}

- (void)pitchByPoints:(double)dy {
    // Also for fun: vertical scroll spins the text about its horizontal axis, tipping it toward or
    // away from the viewer.
    constexpr double kRadPerPoint = 0.004;
    _pitch += dy * kRadPerPoint;
    [self setNeedsDisplay];
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    CGLPixelFormatAttribute attrs[] = {
        kCGLPFAOpenGLProfile,
        (CGLPixelFormatAttribute)kCGLOGLPVersion_GL4_Core,  // 4.1 core -> GLSL 330
        kCGLPFAColorSize,
        (CGLPixelFormatAttribute)24,
        kCGLPFAAlphaSize,
        (CGLPixelFormatAttribute)8,
        kCGLPFAAccelerated,
        kCGLPFADoubleBuffer,
        (CGLPixelFormatAttribute)0,
    };
    CGLPixelFormatObj pf = nullptr;
    GLint npix = 0;
    CGLChoosePixelFormat(attrs, &pf, &npix);
    return pf;
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return YES;
}

// Uploads every unique glyph bitmap into the atlas and builds the static glyph vertex buffer.
- (void)buildResources {
    _program = link_program(kGlyphVS, kGlyphFS);
    _u_proj = glGetUniformLocation(_program, "u_proj");
    _u_tex = glGetUniformLocation(_program, "u_tex");

    _atlas = std::make_unique<Atlas>();

    struct Placement {
        Atlas::UV uv;
        int w;
        int h;
        bool is_color;
    };
    std::map<GlyphKey, Placement> placements;
    for (const auto& [key, bmp] : _source->bitmaps) {
        if (bmp.empty()) continue;
        Atlas::UV uv;
        const int w = static_cast<int>(bmp.width);
        const int h = static_cast<int>(bmp.height);
        if (_atlas->insert(w, h, bmp.pixels, uv)) {
            placements[key] = {uv, w, h, bmp.is_color};
        }
    }

    // One instance per glyph. is_color (as a flag) and the tint color are per-instance fields, so
    // mono and color glyphs draw together in a single instanced call -- no grouping, no per-vertex
    // duplication. Per-glyph syntax colors will just vary the color field.
    std::vector<InstanceData> instances;
    instances.reserve(_source->instances.size());
    for (const auto& inst : _source->instances) {
        auto it = placements.find(inst.key);
        if (it == placements.end()) continue;
        const Placement& p = it->second;
        instances.push_back({
            .x = static_cast<float>(inst.dst_x),
            .y = static_cast<float>(inst.dst_y),
            .w = static_cast<float>(p.w),
            .h = static_cast<float>(p.h),
            .u = p.uv.x,
            .v = p.uv.y,
            .uw = p.uv.w,
            .vh = p.uv.h,
            .r = 0.0f,  // black tint for now
            .g = 0.0f,
            .b = 0.0f,
            .a = 1.0f,
            .flags = p.is_color ? kFlagColor : kFlagMono,
        });
    }
    _glyph_instance_count = static_cast<GLsizei>(instances.size());

    glGenVertexArrays(1, &_glyph_vao);
    glBindVertexArray(_glyph_vao);
    glGenBuffers(1, &_glyph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _glyph_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)),
                 instances.data(), GL_STATIC_DRAW);
    set_instance_attribs();

    // The atlas debug view is a single instance, refreshed each frame from the framebuffer size.
    glGenVertexArrays(1, &_debug_vao);
    glBindVertexArray(_debug_vao);
    glGenBuffers(1, &_debug_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
    set_instance_attribs();

    glBindVertexArray(0);
    _ready = true;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    base::Profiler _("draw");

    CGLSetCurrentContext(glContext);
    if (!_ready) [self buildResources];

    const auto fb_w = static_cast<GLsizei>(self.bounds.size.width * self.contentsScale);
    const auto fb_h = static_cast<GLsizei>(self.bounds.size.height * self.contentsScale);
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // premultiplied over

    glUseProgram(_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _atlas->tex());
    glUniform1i(_u_tex, 0);

    // The atlas HUD uses a plain pixel ortho so it stays pinned. The text rides a full camera
    // instead, which is the whole point of a matrix uniform: scroll, the y-flip, a 3D tilt about
    // the viewport center, and a perspective all compose on the CPU while the shader stays `proj *
    // pos`.
    const float fb_w_f = static_cast<float>(fb_w);
    const float fb_h_f = static_cast<float>(fb_h);
    const float scroll_x = static_cast<float>(_scroll_x);
    const float scroll_y = static_cast<float>(_scroll_y);
    const float cx = fb_w_f * 0.5f;
    const float cy = fb_h_f * 0.5f;

    // Perspective chosen so a fronto-parallel page exactly fills the viewport (identical to the
    // old ortho at zero tilt); the camera sits back by that same distance.
    const float fovy = 50.0f * 3.14159265f / 180.0f;
    const float dist = cy / std::tan(fovy * 0.5f);

    const Mat4 center =
        mul(scale(1.0f, -1.0f, 1.0f), translate(-(cx + scroll_x), -(cy + scroll_y), 0.0f));
    const Mat4 tilt =
        mul(rotate_y(static_cast<float>(_yaw)), rotate_x(static_cast<float>(_pitch)));
    const Mat4 view = translate(0.0f, 0.0f, -dist);
    const Mat4 proj = perspective(fovy, fb_w_f / fb_h_f, 1.0f, dist * 4.0f);
    const Mat4 content_proj = mul(proj, mul(view, mul(tilt, center)));

    const Mat4 screen_proj = ortho(0.0f, fb_w_f, fb_h_f, 0.0f, -1.0f, 1.0f);

    // Text: one instanced draw for every glyph. The per-instance flag selects mono-tint vs color
    // passthrough in the shader, so mono and color glyphs mix freely in a single call.
    glUniformMatrix4fv(_u_proj, 1, GL_FALSE, content_proj.data());
    glBindVertexArray(_glyph_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, _glyph_instance_count);

    // Atlas debug view: one instance covering the whole atlas at 1:1, pinned via screen_proj.
    constexpr float kMargin = 24.0f;
    constexpr float kSide = static_cast<float>(Atlas::kSize);
    const InstanceData debug = {
        .x = static_cast<float>(fb_w) - kMargin - kSide,
        .y = kMargin,
        .w = kSide,
        .h = kSide,
        .u = 0.0f,
        .v = 0.0f,
        .uw = 1.0f,
        .vh = 1.0f,
        .r = 0.0f,
        .g = 0.0f,
        .b = 0.0f,
        .a = 1.0f,
        .flags = kFlagDebug,
    };
    glUniformMatrix4fv(_u_proj, 1, GL_FALSE, screen_proj.data());
    glBindVertexArray(_debug_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _debug_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(debug), &debug);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

@end

// Hosts the GL layer and forwards scroll gestures to it. Scroll events are delivered to the view,
// not the layer, so the translation lives here.
@interface RasterHostView : NSView
@end

@implementation RasterHostView
- (void)scrollWheel:(NSEvent*)event {
    // Precise (trackpad) deltas are already in points; line-based (mouse wheel) deltas are small
    // integers, so scale them to a usable step.
    double dx = event.scrollingDeltaX;
    double dy = event.scrollingDeltaY;
    if (!event.hasPreciseScrollingDeltas) {
        dx *= 10.0;
        dy *= 10.0;
    }
    AtlasGLView* layer = (AtlasGLView*)self.layer;
    [layer scrollByPoints:dy];
    [layer scrollXByPoints:dx];
    // [layer pitchByPoints:dy];
    // [layer yawByPoints:dx];
}
@end

void show_window_gl(const GlyphAtlasSource& source, double scale) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSWindow* window =
        [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1728, 1117)
                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                              NSWindowStyleMaskResizable
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    AtlasGLView* layer = [[AtlasGLView alloc] initWithSource:&source];
    layer.contentsScale = scale;  // 2x backing on Retina
    layer.needsDisplayOnBoundsChange = YES;
    layer.opaque = YES;

    RasterHostView* view = [[RasterHostView alloc] initWithFrame:NSMakeRect(0, 0, 1728, 1117)];
    view.layer = layer;
    view.wantsLayer = YES;
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
    [layer setNeedsDisplay];

    [NSApp run];
}
