#include "base/strings/sys_string_conversions.h"
#include "experiments/rasterizer/atlas.h"
#include "experiments/rasterizer/gl_helpers.h"
#include "gl/loader.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <format>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <spng.h>
#include <string>
#include <utility>
#include <vector>
#include <shellscalingapi.h>
#include <windows.h>

// Windows counterpart of mac/gl_renderer.mm: the same instanced glyph pipeline and the same
// shaders, on a Win32 window with a WGL core context. Deliberately narrower than the macOS
// renderer -- no 3D tilt, no trackpad axis lock -- because its job is to put DirectWrite output on
// screen next to Sublime Text's, not to be a playground.

namespace {

const char* kGlyphVS =
#include "experiments/rasterizer/glyph_vert.glsl"
    ;
const char* kGlyphFS =
#include "experiments/rasterizer/glyph_frag.glsl"
    ;

// Matches mac/gl_renderer.mm's InstanceData and glyph_vert.glsl's four attributes.
struct InstanceData {
    float x, y, w, h;    // rect in device pixels, top-left origin
    float u, v, uw, vh;  // atlas uv rect
    float r, g, b, a;    // tint for mono glyphs; ignored otherwise
    float flags;
};
static_assert(sizeof(InstanceData) == 13 * sizeof(float));

constexpr float kFlagMono = 0.0f;
constexpr float kFlagColor = 1.0f;

using Mat4 = std::array<float, 16>;

// Column-major, to match glUniformMatrix4fv(transpose = GL_FALSE). Passing top < bottom gives a
// y-down, top-left-origin pixel space.
Mat4 ortho(float left, float right, float bottom, float top) {
    const float rl = right - left;
    const float tb = top - bottom;
    // clang-format off
    return {
        2.0f / rl,            0.0f,                 0.0f,  0.0f,
        0.0f,                 2.0f / tb,            0.0f,  0.0f,
        0.0f,                 0.0f,                -1.0f,  0.0f,
        -(right + left) / rl, -(top + bottom) / tb,  0.0f,  1.0f,
    };
    // clang-format on
}

GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        spdlog::error("shader compile failed: {}", log);
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
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {0};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        spdlog::error("program link failed: {}", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

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

// ---------------------------------------------------------------------------------------------
// WGL context creation

constexpr int kWglContextMajorVersionArb = 0x2091;
constexpr int kWglContextMinorVersionArb = 0x2092;
constexpr int kWglContextProfileMaskArb = 0x9126;
constexpr int kWglContextCoreProfileBitArb = 0x00000001;

using PFN_wglCreateContextAttribsARB = HGLRC(WINAPI*)(HDC, HGLRC, const int*);

void set_pixel_format(HDC dc) {
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd);
}

// A 4.1 core context needs wglCreateContextAttribsARB, which can only be resolved through a
// context that already exists -- hence the throwaway window and legacy context first.
HGLRC create_core_context(HDC dc) {
    HWND dummy = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr,
                                 GetModuleHandleW(nullptr), nullptr);
    HDC dummy_dc = GetDC(dummy);
    set_pixel_format(dummy_dc);
    HGLRC dummy_ctx = wglCreateContext(dummy_dc);
    wglMakeCurrent(dummy_dc, dummy_ctx);

    auto create_attribs = reinterpret_cast<PFN_wglCreateContextAttribsARB>(
        wglGetProcAddress("wglCreateContextAttribsARB"));

    HGLRC context = nullptr;
    if (create_attribs) {
        const int attribs[] = {
            kWglContextMajorVersionArb,
            4,
            kWglContextMinorVersionArb,
            1,
            kWglContextProfileMaskArb,
            kWglContextCoreProfileBitArb,
            0,
        };
        context = create_attribs(dc, nullptr, attribs);
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummy_ctx);
    ReleaseDC(dummy, dummy_dc);
    DestroyWindow(dummy);

    // Without the extension, take whatever wglCreateContext gives: drivers usually return their
    // highest supported version in the compatibility profile, which still runs `#version 410`.
    if (!context) context = wglCreateContext(dc);
    return context;
}

bool make_current_and_load(HDC dc, HGLRC context) {
    if (!context || !wglMakeCurrent(dc, context)) {
        spdlog::error("could not create an OpenGL context");
        return false;
    }
    gl::load_global_function_pointers();

    const auto* version =
        glGetString ? reinterpret_cast<const char*>(glGetString(GL_VERSION)) : nullptr;
    int major = 0;
    int minor = 0;
    const bool version_ok = version && std::sscanf(version, "%d.%d", &major, &minor) == 2 &&
                            (major > 4 || (major == 4 && minor >= 1));
    if (!version_ok || !glGenVertexArrays || !glDrawArraysInstanced || !glVertexAttribDivisor ||
        !glUniformMatrix4fv || !glGenFramebuffers) {
        spdlog::error("OpenGL 4.1 is required; this context reports \"{}\"",
                      version ? version : "?");
        return false;
    }
    spdlog::info("OpenGL {}", version ? version : "?");
    return true;
}

// ---------------------------------------------------------------------------------------------

// Owns the shader program, the instance buffer and the glyph atlas. Content is swapped with
// set_source(); who decides what to show lives outside, as on macOS.
class Renderer {
public:
    void set_up() {
        program_ = link_program(kGlyphVS, kGlyphFS);
        u_proj_ = glGetUniformLocation(program_, "u_proj");
        u_tex_ = glGetUniformLocation(program_, "u_tex");

        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        set_instance_attribs();
        glBindVertexArray(0);
    }

    void set_source(GlyphAtlasSource source) {
        // A fresh atlas each time; the previous texture goes with the old one.
        atlas_ = std::make_unique<Atlas>();

        struct Placement {
            Atlas::UV uv;
            int w, h;
            bool colored;
        };
        std::map<GlyphKey, Placement> placements;
        for (const auto& [key, bmp] : source.bitmaps) {
            if (bmp.empty()) continue;
            Atlas::UV uv;
            const int w = static_cast<int>(bmp.width);
            const int h = static_cast<int>(bmp.height);
            if (atlas_->insert(w, h, bmp.pixels, uv)) placements[key] = {uv, w, h, bmp.colored};
        }

        std::vector<InstanceData> instances;
        instances.reserve(source.instances.size());
        for (const auto& inst : source.instances) {
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
        instance_count_ = static_cast<GLsizei>(instances.size());

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)),
                     instances.data(), GL_DYNAMIC_DRAW);
    }

    void draw(int fb_w, int fb_h, double scroll_x, double scroll_y) const {
        glViewport(0, 0, fb_w, fb_h);
        glClearColor(252 / 255.f, 253 / 255.f, 253 / 255.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (!atlas_) return;

        glEnable(GL_BLEND);
        // Premultiplied over, with the second fragment output supplying the coverage so each
        // channel blends independently -- see glyph_frag.glsl.
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC1_COLOR, GL_ONE,
                            GL_ONE_MINUS_SRC1_ALPHA);

        glUseProgram(program_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas_->tex());
        glUniform1i(u_tex_, 0);

        // Scroll folds into the projection: the visible window is the page shifted by the scroll
        // offset, in device pixels.
        const float sx = static_cast<float>(scroll_x);
        const float sy = static_cast<float>(scroll_y);
        const Mat4 proj =
            ortho(sx, sx + static_cast<float>(fb_w), sy + static_cast<float>(fb_h), sy);
        glUniformMatrix4fv(u_proj_, 1, GL_FALSE, proj.data());
        glBindVertexArray(vao_);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, instance_count_);
    }

private:
    GLuint program_ = 0;
    GLint u_proj_ = -1;
    GLint u_tex_ = -1;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    std::unique_ptr<Atlas> atlas_;
    GLsizei instance_count_ = 0;
};

// ---------------------------------------------------------------------------------------------
// Interactive window

// The layout is produced in device pixels, so the page is this big whatever the display can show;
// the window is clamped to the work area and scrolls.
constexpr int kPageWidthPoints = 1728;
constexpr int kPageHeightPoints = 1117;

struct WindowState {
    Renderer renderer;
    SourceProvider provider;
    std::vector<std::string> families;
    size_t family_index = 0;
    font::FontSpec spec;
    double scroll_x = 0;
    double scroll_y = 0;
    int fb_w = 0;
    int fb_h = 0;
    double scale = 1;
    HDC dc = nullptr;
    HWND window = nullptr;

    void relayout() {
        spec.family = families[family_index];
        renderer.set_source(provider(spec));

        // There is no console under the windowed subsystem, so the title bar is the only place a
        // screenshot can show which font it is actually looking at.
        const UINT dpi = GetDpiForWindow(window);
        const std::string title =
            std::format("rasterizer -- {}  em {:g} DIP x{:g} = {:g}px  {} DPI ({:g}%){}{}", spec.family,
                        spec.size, scale, spec.size * scale, dpi,
                        dpi * 100.0 / USER_DEFAULT_SCREEN_DPI,
                        spec.weight == font::Weight::Bold ? "  bold" : "",
                        spec.slant == font::Slant::Italic ? "  italic" : "");
        SetWindowTextW(window, base::sys_utf8_to_wide(title).c_str());
    }
};

WindowState* state_of(HWND window) {
    return reinterpret_cast<WindowState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    WindowState* state = state_of(window);
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        if (state) {
            state->renderer.draw(state->fb_w, state->fb_h, state->scroll_x, state->scroll_y);
            SwapBuffers(state->dc);
        }
        ValidateRect(window, nullptr);
        return 0;
    case WM_SIZE:
        if (state) {
            state->fb_w = LOWORD(lparam);
            state->fb_h = HIWORD(lparam);
        }
        return 0;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
        if (!state) break;
        const double delta = GET_WHEEL_DELTA_WPARAM(wparam) / static_cast<double>(WHEEL_DELTA);
        constexpr double kStepPixels = 40.0;
        // Whole device pixels only, so glyphs stay texel-aligned and don't shimmer mid-scroll.
        if (message == WM_MOUSEWHEEL) state->scroll_y -= std::round(delta * kStepPixels);
        else state->scroll_x += std::round(delta * kStepPixels);
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    case WM_CHAR: {
        if (!state) break;
        const int n = static_cast<int>(state->families.size());
        switch (wparam) {
        case '-':
            state->spec.size = std::clamp(state->spec.size - 0.5, 4.0, 400.0);
            break;
        case '+':
        case '=':
            state->spec.size = std::clamp(state->spec.size + 0.5, 4.0, 400.0);
            break;
        case '[':
            state->family_index =
                static_cast<size_t>((static_cast<int>(state->family_index) + n - 1) % n);
            break;
        case ']':
            state->family_index =
                static_cast<size_t>((static_cast<int>(state->family_index) + 1) % n);
            break;
        case 'b':
            state->spec.weight = state->spec.weight == font::Weight::Bold ? font::Weight::Normal
                                                                          : font::Weight::Bold;
            break;
        case 'i':
            state->spec.slant = state->spec.slant == font::Slant::Italic ? font::Slant::Normal
                                                                         : font::Slant::Italic;
            break;
        case 'q':
        case 27:
            PostQuitMessage(0);
            return 0;
        default:
            return 0;
        }
        state->relayout();
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

// Registers the window class once and creates a hidden window whose client area is `width` x
// `height` device pixels, clamped to the work area. The interactive window opens maximized, so that
// size is only what it restores down to.
HWND create_window(const wchar_t* title, int width, int height) {
    static const ATOM registered = [] {
        // Without this the process is DPI-virtualised: Windows hands it a 96-DPI-sized client area
        // and DWM bitmap-scales the result up, which blurs every glyph. Must happen before the
        // first window exists.
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = window_proc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"RasterizerWindow";
        return RegisterClassExW(&wc);
    }();
    if (!registered) return nullptr;

    RECT work = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    RECT rect = {0, 0, std::min<LONG>(width, work.right - work.left),
                 std::min<LONG>(height, work.bottom - work.top)};
    const DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE);

    return CreateWindowExW(0, L"RasterizerWindow", title, style, CW_USEDEFAULT, CW_USEDEFAULT,
                           rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
                           GetModuleHandleW(nullptr), nullptr);
}

// ---------------------------------------------------------------------------------------------
// Offscreen capture for the screenshot suite

bool write_png(const char* path, int width, int height, const std::vector<uint8_t>& rgba) {
    FILE* file = std::fopen(path, "wb");
    if (!file) {
        spdlog::error("cannot write {}", path);
        return false;
    }
    spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);
    spng_set_png_file(ctx, file);
    spng_ihdr ihdr = {};
    ihdr.width = static_cast<uint32_t>(width);
    ihdr.height = static_cast<uint32_t>(height);
    ihdr.bit_depth = 8;
    ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
    spng_set_ihdr(ctx, &ihdr);
    const int err =
        spng_encode_image(ctx, rgba.data(), rgba.size(), SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
    spng_ctx_free(ctx);
    std::fclose(file);
    if (err) spdlog::error("png encode failed for {}: {}", path, spng_strerror(err));
    return err == 0;
}

// Reads the current framebuffer and writes `crop` out as a PNG. glReadPixels is bottom-up, so the
// rows come back reversed and the crop's y is measured from the top.
bool capture_to_png(int fb_w, int fb_h, Crop crop, const char* path) {
    if (crop.w <= 0 || crop.h <= 0) crop = {0, 0, fb_w, fb_h};
    crop.w = std::min(crop.w, fb_w - crop.x);
    crop.h = std::min(crop.h, fb_h - crop.y);
    if (crop.w <= 0 || crop.h <= 0) return false;

    std::vector<uint8_t> frame(static_cast<size_t>(fb_w) * static_cast<size_t>(fb_h) * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, frame.data());

    const size_t row_bytes = static_cast<size_t>(crop.w) * 4;
    std::vector<uint8_t> out(row_bytes * static_cast<size_t>(crop.h));
    for (int y = 0; y < crop.h; y++) {
        const size_t src_row = static_cast<size_t>(fb_h - 1 - (crop.y + y));
        const size_t src =
            src_row * static_cast<size_t>(fb_w) * 4 + static_cast<size_t>(crop.x) * 4;
        std::copy_n(frame.begin() + static_cast<ptrdiff_t>(src), row_bytes,
                    out.begin() + static_cast<ptrdiff_t>(static_cast<size_t>(y) * row_bytes));
    }
    return write_png(path, crop.w, crop.h, out);
}

}  // namespace

void run_text_window(const font::FontSpec& initial,
                     std::vector<std::string> families,
                     double scale,
                     SourceProvider provider) {
    const int page_w = static_cast<int>(kPageWidthPoints * scale);
    const int page_h = static_cast<int>(kPageHeightPoints * scale);
    HWND window = create_window(L"rasterizer", page_w, page_h);
    if (!window) return;

    HDC dc = GetDC(window);
    set_pixel_format(dc);
    if (!make_current_and_load(dc, create_core_context(dc))) return;

    WindowState state;
    state.provider = std::move(provider);
    state.families = std::move(families);
    state.spec = initial;
    state.scale = scale;
    state.dc = dc;
    state.window = window;
    RECT client = {};
    GetClientRect(window, &client);
    state.fb_w = client.right;
    state.fb_h = client.bottom;
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));

    state.renderer.set_up();
    state.relayout();
    ShowWindow(window, SW_SHOWMAXIMIZED);

    // Drawing happens in WM_PAINT rather than here, so the window is always still alive when the
    // renderer touches its DC, and idle frames cost nothing.
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    SetWindowLongPtrW(window, GWLP_USERDATA, 0);
}

void run_test_window(std::vector<TestShot> shots, Crop crop, double scale) {
    const int page_w = static_cast<int>(kPageWidthPoints * scale);
    const int page_h = static_cast<int>(kPageHeightPoints * scale);

    // Rendering offscreen keeps the page at its full device-pixel size no matter how small the
    // display is, and skips the compositor entirely -- the macOS path has to capture the window
    // and wait for each frame to settle, but here the read-back is the frame.
    HWND window = create_window(L"rasterizer (offscreen)", 1, 1);
    if (!window) return;
    HDC dc = GetDC(window);
    set_pixel_format(dc);
    if (!make_current_and_load(dc, create_core_context(dc))) return;

    GLuint color = 0;
    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, page_w, page_h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("offscreen framebuffer is incomplete at {}x{}", page_w, page_h);
        return;
    }

    Renderer renderer;
    renderer.set_up();

    for (size_t i = 0; i < shots.size(); i++) {
        const TestShot& shot = shots[i];
        auto handle = font::create_font(shot.font);
        if (!handle) {
            spdlog::error("skipping \"{}\": could not create font \"{}\"", shot.out_path,
                          shot.font.family);
            continue;
        }
        renderer.set_source(layout_text(*handle, shot.lines, scale));
        renderer.draw(page_w, page_h, 0, 0);
        const bool ok = capture_to_png(page_w, page_h, crop, shot.out_path.c_str());
        spdlog::info("[{}/{}] {}{}", i + 1, shots.size(), shot.out_path, ok ? "" : "  (FAILED)");
    }

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
}
