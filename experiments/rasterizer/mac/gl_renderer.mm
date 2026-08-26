#include "base/debug/profiler.h"
#include "experiments/rasterizer/atlas.h"
#include "experiments/rasterizer/mac/capture.h"
#include "experiments/rasterizer/gl_helpers.h"
#include <AppKit/AppKit.h>
#include <OpenGL/OpenGL.h>
#include <QuartzCore/QuartzCore.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
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

// Sublime Text-style scroll axis-lock: snaps near-horizontal / near-vertical trackpad scrolling to
// a single axis so jitter on the other axis vanishes, while leaving deliberate diagonal scrolling
// free. The lock is decided from the summed absolute deltas over a trailing time window, not the
// current event, so brief perpendicular wobble is outweighed by the dominant axis yet an
// intentional curve into a diagonal releases within the window. Recovered from Sublime Text's
// scroll_area_control::handle_event; see reverse_engineering/.
class ScrollAxisLock {
public:
    // `now` is a monotonic timestamp in seconds. Returns the delta with the off-axis component
    // zeroed while locked. Call with gesture_ended once per gesture end to reset the window.
    std::pair<double, double> apply(double dx, double dy, double now, bool gesture_ended) {
        if (gesture_ended) {
            samples_.clear();
            sum_x_ = sum_y_ = 0.0;
            return {dx, dy};
        }
        // Keep the |Δ| sums over the trailing window incrementally (add on push, subtract on evict)
        // so each event is O(1). The running error is bounded by the gesture and reset on its end.
        while (!samples_.empty() && samples_.front().t + kWindow < now) {
            sum_x_ -= std::abs(samples_.front().dx);
            sum_y_ -= std::abs(samples_.front().dy);
            samples_.pop_front();
        }
        samples_.push_back({now, dx, dy});
        sum_x_ += std::abs(dx);
        sum_y_ += std::abs(dy);

        // Treat (sum_x, sum_y) as a first-quadrant vector: lock to whichever axis it falls within
        // kCone of, comparing against its Euclidean length so the middle wedge is a genuine diagonal
        // dead-zone.
        const double mag = std::sqrt(sum_x_ * sum_x_ + sum_y_ * sum_y_);
        if (sum_x_ > kCone * mag) return {dx, 0.0};
        if (sum_y_ > kCone * mag) return {0.0, dy};
        return {dx, dy};
    }

private:
    static constexpr double kWindow = 0.2;  // seconds of history the lock decision considers
    static constexpr double kCone = 0.85;   // cos(31.8 deg): half-angle of each axis's lock cone
    struct Sample {
        double t, dx, dy;
    };
    std::deque<Sample> samples_;
    double sum_x_ = 0.0, sum_y_ = 0.0;
};

}  // namespace

// Renders a page of text (a GlyphAtlasSource) as one textured quad per glyph, sampled from a glyph
// atlas, plus the whole atlas in the upper-right corner for debugging. Content is swapped with
// setSource:; who decides what to show (interactive font control vs the test suite) lives outside.
// Static GL objects are built once when a context first exists; the atlas and glyph geometry are
// (re)uploaded whenever the source changes.
@interface AtlasGLView : CAOpenGLLayer {
    GlyphAtlasSource _source;

    double _scroll_x;       // content scrolled left past the left edge, in device pixels
    double _scroll_y;       // content scrolled up past the top edge, in device pixels
    double _scroll_frac_x;  // sub-pixel remainder carried between events so _scroll_x stays integral
    double _scroll_frac_y;  // sub-pixel remainder carried between events so _scroll_y stays integral
    double _yaw;            // text tilt about the vertical axis, in radians
    double _pitch;          // text tilt about the horizontal axis, in radians (unbound)

    // Static GL objects, built once on the first draw.
    bool _gl_ready;
    GLuint _program;
    GLint _u_proj;
    GLint _u_tex;
    GLuint _glyph_vao;
    GLuint _glyph_vbo;  // per-glyph InstanceData
    GLuint _debug_vao;
    GLuint _debug_vbo;  // one InstanceData for the atlas debug quad, refreshed per frame

    // Content GL state, rebuilt from `_source` whenever it changes.
    bool _needs_upload;
    std::unique_ptr<Atlas> _atlas;
    GLsizei _glyph_instance_count;
}
- (void)setSource:(GlyphAtlasSource)source;
- (void)scrollByPoints:(double)dy;   // dy in view points; positive scrolls content down
- (void)scrollXByPoints:(double)dx;  // dx in view points; positive scrolls content right
- (void)yawByPoints:(double)dx;      // dx in view points; tilts the text about the vertical axis
- (void)pitchByPoints:(double)dy;    // dy in view points; tilts the text about the horizontal axis
@end

@implementation AtlasGLView

- (void)setSource:(GlyphAtlasSource)source {
    _source = std::move(source);
    _needs_upload = true;
    [self setNeedsDisplay];
}

- (void)scrollByPoints:(double)dy {
    // Points -> device pixels so the content tracks the gesture 1:1. Apply only whole device pixels
    // and carry the sub-pixel remainder into the next event: this keeps the page texel-aligned so
    // glyphs don't resample and shimmer mid-scroll, while the carry preserves smooth 1:1 tracking.
    // Sublime's scroll_area_control::handle_event does the equivalent via its set/read-back position
    // leftover (see reverse_engineering/).
    const double want = _scroll_frac_y - dy * self.contentsScale;
    const double step = std::round(want);
    _scroll_frac_y = want - step;
    _scroll_y += step;
    [self setNeedsDisplay];
}

- (void)scrollXByPoints:(double)dx {
    // Horizontal counterpart of scrollByPoints:, with the same whole-pixel snap and remainder carry.
    const double want = _scroll_frac_x - dx * self.contentsScale;
    const double step = std::round(want);
    _scroll_frac_x = want - step;
    _scroll_x += step;
    [self setNeedsDisplay];
}

- (void)yawByPoints:(double)dx {
    // Just for fun: horizontal scroll spins the text about its vertical axis.
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

// Builds the static GL objects that don't depend on content: the shader program and the two vertex
// arrays. Attributes are bound here (the buffers need not hold data yet); the buffers are filled
// by uploadSource and the per-frame debug refresh.
- (void)setUpGL {
    _program = link_program(kGlyphVS, kGlyphFS);
    _u_proj = glGetUniformLocation(_program, "u_proj");
    _u_tex = glGetUniformLocation(_program, "u_tex");

    glGenVertexArrays(1, &_glyph_vao);
    glBindVertexArray(_glyph_vao);
    glGenBuffers(1, &_glyph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _glyph_vbo);
    set_instance_attribs();

    // The atlas debug view is a single instance, refreshed each frame from the framebuffer size.
    glGenVertexArrays(1, &_debug_vao);
    glBindVertexArray(_debug_vao);
    glGenBuffers(1, &_debug_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
    set_instance_attribs();

    glBindVertexArray(0);
    _gl_ready = true;
}

// Packs the current source's unique bitmaps into a fresh atlas and (re)uploads one InstanceData
// per glyph. Called on the first draw and after every setSource:.
- (void)uploadSource {
    // A fresh atlas each time; the previous texture is freed with the old Atlas.
    _atlas = std::make_unique<Atlas>();

    struct Placement {
        Atlas::UV uv;
        int w;
        int h;
        bool colored;
    };
    std::map<GlyphKey, Placement> placements;
    for (const auto& [key, bmp] : _source.bitmaps) {
        if (bmp.empty()) continue;
        Atlas::UV uv;
        const int w = static_cast<int>(bmp.width);
        const int h = static_cast<int>(bmp.height);
        if (_atlas->insert(w, h, bmp.pixels, uv)) {
            placements[key] = {uv, w, h, bmp.colored};
        }
    }

    // colored (as a flag) and the tint color are per-instance fields, so mono and color glyphs
    // draw together in a single instanced call -- no grouping, no per-vertex duplication.
    // Per-glyph syntax colors will just vary the color field.
    std::vector<InstanceData> instances;
    instances.reserve(_source.instances.size());
    for (const auto& inst : _source.instances) {
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
            .r = 51 / 255.f,
            .g = 51 / 255.f,
            .b = 51 / 255.f,
            .a = 1.0f,
            .flags = p.colored ? kFlagColor : kFlagMono,
        });
    }
    _glyph_instance_count = static_cast<GLsizei>(instances.size());

    glBindBuffer(GL_ARRAY_BUFFER, _glyph_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)),
                 instances.data(), GL_DYNAMIC_DRAW);

    _needs_upload = false;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    base::Profiler _("draw");

    CGLSetCurrentContext(glContext);
    if (!_gl_ready) [self setUpGL];
    if (_needs_upload) [self uploadSource];

    const auto fb_w = static_cast<GLsizei>(self.bounds.size.width * self.contentsScale);
    const auto fb_h = static_cast<GLsizei>(self.bounds.size.height * self.contentsScale);
    glViewport(0, 0, fb_w, fb_h);
    glClearColor(252 / 255.f, 253 / 255.f, 253 / 255.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Nothing to draw until a source has been uploaded (the test window's priming frame).
    if (_atlas) {
        glEnable(GL_BLEND);
        // Premultiplied over, with the second fragment output supplying the coverage so each
        // channel blends independently -- see glyph_frag.glsl.
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_COLOR, GL_ONE,
                            GL_ONE_MINUS_SRC1_ALPHA);

        glUseProgram(_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _atlas->tex());
        glUniform1i(_u_tex, 0);

        // The atlas HUD uses a plain pixel ortho so it stays pinned. The text rides a full camera
        // instead, which is the whole point of a matrix uniform: scroll, the y-flip, a 3D tilt
        // about the viewport center, and a perspective all compose on the CPU while the shader
        // stays `proj * pos`.
        const float fb_w_f = static_cast<float>(fb_w);
        const float fb_h_f = static_cast<float>(fb_h);
        const float scroll_x = static_cast<float>(_scroll_x);
        const float scroll_y = static_cast<float>(_scroll_y);
        const float cx = fb_w_f * 0.5f;
        const float cy = fb_h_f * 0.5f;

        // Perspective chosen so a fronto-parallel page exactly fills the viewport (identical to
        // the old ortho at zero tilt); the camera sits back by that same distance.
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

        // Text: one instanced draw for every glyph. The per-instance flag selects mono-tint vs
        // color passthrough in the shader, so mono and color glyphs mix freely in a single call.
        glUniformMatrix4fv(_u_proj, 1, GL_FALSE, content_proj.data());
        glBindVertexArray(_glyph_vao);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, _glyph_instance_count);

        // Atlas debug view: one instance covering the whole atlas at 1:1, pinned via screen_proj.
        if constexpr (Atlas::kRenderDebugView) {
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
        }
    }

    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

@end

// Hosts the GL layer for interactive use: owns the current font and forwards input. Scroll and key
// events are delivered to the view, not the layer, so the font control lives here. On any change
// it re-lays-out via `_provider` and pushes the new page to the layer.
@interface RasterHostView : NSView {
    SourceProvider _provider;
    std::vector<std::string> _families;
    size_t _family_index;
    double _size;
    font::Weight _weight;
    font::Slant _slant;
    ScrollAxisLock _scroll_lock;
}
- (instancetype)initWithFrame:(NSRect)frame
                         spec:(const font::FontSpec&)spec
                     families:(std::vector<std::string>)families
                     provider:(SourceProvider)provider;
- (void)relayout;  // lay out the current font and push it to the layer
@end

@implementation RasterHostView

- (instancetype)initWithFrame:(NSRect)frame
                         spec:(const font::FontSpec&)spec
                     families:(std::vector<std::string>)families
                     provider:(SourceProvider)provider {
    if (self = [super initWithFrame:frame]) {
        _provider = std::move(provider);
        _families = std::move(families);
        _family_index = 0;  // families.front() == spec.family by contract
        _size = spec.size;
        _weight = spec.weight;
        _slant = spec.slant;
    }
    return self;
}

- (void)relayout {
    AtlasGLView* layer = (AtlasGLView*)self.layer;
    [layer setSource:_provider({_families[_family_index], _size, _weight, _slant})];
}

- (void)changeSizeBy:(double)delta {
    _size = std::clamp(_size + delta, 4.0, 400.0);
    [self relayout];
}

- (void)cycleFamilyBy:(int)dir {
    const int n = static_cast<int>(_families.size());
    _family_index = static_cast<size_t>(((static_cast<int>(_family_index) + dir) % n + n) % n);
    [self relayout];
}

- (void)toggleBold {
    _weight = _weight == font::Weight::Bold ? font::Weight::Normal : font::Weight::Bold;
    [self relayout];
}

- (void)toggleItalic {
    _slant = _slant == font::Slant::Italic ? font::Slant::Normal : font::Slant::Italic;
    [self relayout];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)scrollWheel:(NSEvent*)event {
    // Precise (trackpad) deltas are already in points; line-based (mouse wheel) deltas are small
    // integers, so scale them to a usable step.
    double dx = event.scrollingDeltaX;
    double dy = event.scrollingDeltaY;
    if (!event.hasPreciseScrollingDeltas) {
        dx *= 10.0;
        dy *= 10.0;
    }
    const bool ended = (event.phase & (NSEventPhaseEnded | NSEventPhaseCancelled)) != 0;
    const auto locked = _scroll_lock.apply(dx, dy, event.timestamp, ended);
    dx = locked.first;
    dy = locked.second;
    AtlasGLView* layer = (AtlasGLView*)self.layer;
    [layer scrollByPoints:dy];
    [layer scrollXByPoints:dx];
    // [layer pitchByPoints:dy];
    // [layer yawByPoints:dx];
}

- (void)keyDown:(NSEvent*)event {
    NSString* chars = event.charactersIgnoringModifiers;
    switch (chars.length ? [chars characterAtIndex:0] : 0) {
    case '-':
        [self changeSizeBy:-0.5];
        break;
    case '+':
    case '=':
        [self changeSizeBy:0.5];
        break;
    case '[':
        [self cycleFamilyBy:-1];
        break;
    case ']':
        [self cycleFamilyBy:1];
        break;
    case 'b':
        [self toggleBold];
        break;
    case 'i':
        [self toggleItalic];
        break;
    default:
        [super keyDown:event];
        break;
    }
}

@end

// Builds a window hosting a fresh AtlasGLView and returns the layer. `borderless` gives the test
// suite a chrome-free window whose backing store is exactly the content (so crop rects are
// content-relative).
static AtlasGLView* attach_layer(NSWindow* window, NSView* view, double scale) {
    AtlasGLView* layer = [[AtlasGLView alloc] init];
    layer.contentsScale = scale;  // 2x backing on Retina
    layer.needsDisplayOnBoundsChange = YES;
    layer.opaque = YES;
    view.layer = layer;
    view.wantsLayer = YES;
    window.contentView = view;
    return layer;
}

void run_text_window(const font::FontSpec& initial,
                     std::vector<std::string> families,
                     double scale,
                     SourceProvider provider) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, 1728, 1117);
    NSWindow* window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                              NSWindowStyleMaskResizable
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    RasterHostView* view = [[RasterHostView alloc] initWithFrame:frame
                                                            spec:initial
                                                        families:std::move(families)
                                                        provider:std::move(provider)];
    AtlasGLView* layer = attach_layer(window, view, scale);

    [window center];
    [window makeKeyAndOrderFront:nil];
    [window makeFirstResponder:view];
    [view relayout];  // initial page, now that the layer is wired up

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@""];
    [app_menu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"]];
    app_item.submenu = app_menu;
    [main_menu addItem:app_item];
    NSApp.mainMenu = main_menu;

    // [NSApp activateIgnoringOtherApps:YES];
    [layer setNeedsDisplay];

    [NSApp run];
}

void run_test_window(std::vector<TestShot> shots, Crop crop, double scale) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Borderless so the window's backing store is exactly the content: crop rects are then simply
    // content-relative device pixels.
    NSRect frame = NSMakeRect(0, 0, 1728, 1117);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:NSWindowStyleMaskBorderless
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.releasedWhenClosed = NO;
    AtlasGLView* layer = attach_layer(window, [[NSView alloc] initWithFrame:frame], scale);

    [window setFrameOrigin:NSMakePoint(0, 0)];
    [window makeKeyAndOrderFront:nil];
    // [NSApp activateIgnoringOtherApps:YES];

    const uint32_t window_id = static_cast<uint32_t>(window.windowNumber);
    const capture::Crop capture_crop = {crop.x, crop.y, crop.w, crop.h};

    // Prime the empty window so the first shot has a baseline to change from.
    for (int i = 0; i < 8; i++) capture::pump(0.008);
    capture::Frame baseline = capture::capture_frame(window_id, capture_crop);

    for (size_t i = 0; i < shots.size(); i++) {
        const TestShot& shot = shots[i];
        auto handle = font::create_font(shot.font);
        if (!handle) {
            spdlog::error("skipping \"{}\": could not create font \"{}\"", shot.out_path,
                          shot.font.family);
            continue;
        }
        [layer setSource:layout_text(*handle, shot.lines, scale)];

        capture::Frame settled = capture::wait_settled(window_id, capture_crop, baseline);
        bool ok = settled && capture::frame_to_png(settled, shot.out_path.c_str());
        spdlog::info("[{}/{}] {}{}", i + 1, shots.size(), shot.out_path, ok ? "" : "  (FAILED)");
        capture::release_frame(baseline);
        baseline = settled;
    }
    capture::release_frame(baseline);
}
