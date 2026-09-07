#include "px/px_gl.h"
#include "px/win/px_win_private.h"
#include <cstdio>
#include <print>

PFN_glCreateShader px_glCreateShader = nullptr;
PFN_glShaderSource px_glShaderSource = nullptr;
PFN_glCompileShader px_glCompileShader = nullptr;
PFN_glGetShaderiv px_glGetShaderiv = nullptr;
PFN_glGetShaderInfoLog px_glGetShaderInfoLog = nullptr;
PFN_glDeleteShader px_glDeleteShader = nullptr;
PFN_glCreateProgram px_glCreateProgram = nullptr;
PFN_glAttachShader px_glAttachShader = nullptr;
PFN_glBindAttribLocation px_glBindAttribLocation = nullptr;
PFN_glLinkProgram px_glLinkProgram = nullptr;
PFN_glGetProgramiv px_glGetProgramiv = nullptr;
PFN_glGetProgramInfoLog px_glGetProgramInfoLog = nullptr;
PFN_glDeleteProgram px_glDeleteProgram = nullptr;
PFN_glUseProgram px_glUseProgram = nullptr;
PFN_glGetUniformLocation px_glGetUniformLocation = nullptr;
PFN_glUniform2f px_glUniform2f = nullptr;
PFN_glUniform1f px_glUniform1f = nullptr;
PFN_glUniform1i px_glUniform1i = nullptr;
PFN_glBlendFuncSeparate px_glBlendFuncSeparate = nullptr;
PFN_glActiveTexture px_glActiveTexture = nullptr;
PFN_glGenVertexArrays px_glGenVertexArrays = nullptr;
PFN_glBindVertexArray px_glBindVertexArray = nullptr;
PFN_glGenBuffers px_glGenBuffers = nullptr;
PFN_glBindBuffer px_glBindBuffer = nullptr;
PFN_glBufferData px_glBufferData = nullptr;
PFN_glBufferSubData px_glBufferSubData = nullptr;
PFN_glTexBuffer px_glTexBuffer = nullptr;
PFN_glDrawArraysInstanced px_glDrawArraysInstanced = nullptr;
PFN_glEnableVertexAttribArray px_glEnableVertexAttribArray = nullptr;
PFN_glVertexAttribPointer px_glVertexAttribPointer = nullptr;

namespace {

using PFN_wglGetProcAddress = PROC(WINAPI*)(LPCSTR);

HGLRC g_shared_context = nullptr;
bool g_has_shaders = false;

// Set once the driver has been rejected, so later windows go straight to the software path instead
// of creating and tearing down a context each.
bool g_gl_unusable = false;

PFN_wglGetProcAddress g_get_proc_address = nullptr;

// TODO: Get rid of stencil (`pfd.cStencilBits = 0`). Also understand the lack of PFD_DOUBLEBUFFER.
PIXELFORMATDESCRIPTOR describe_pixel_format() {
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    return pfd;
}

// Every name loaded here postdates 1.1, so opengl32.dll's own export table cannot answer for any
// of them and only the driver's wglGetProcAddress will do.
template <typename T>
bool load(T* slot, const char* name) {
    *slot = reinterpret_cast<T>(g_get_proc_address(name));
    if (*slot) {
        return true;
    }
    std::println(stderr, "px: failed to load OpenGL function: {}", name);
    return false;
}

// Returns false at the first entry point the driver cannot supply, as ST's loader does.
bool load_modern_gl() {
    HMODULE gl = LoadLibraryA("opengl32.dll");
    if (!gl) {
        std::println(stderr, "px: failed to load opengl32.dll (error {})", GetLastError());
        return false;
    }
    g_get_proc_address =
        reinterpret_cast<PFN_wglGetProcAddress>(GetProcAddress(gl, "wglGetProcAddress"));
    if (!g_get_proc_address) {
        std::println(stderr, "px: opengl32.dll exports no wglGetProcAddress");
        return false;
    }

    return load(&px_glCreateShader, "glCreateShader") &&
           load(&px_glShaderSource, "glShaderSource") &&
           load(&px_glCompileShader, "glCompileShader") &&
           load(&px_glGetShaderiv, "glGetShaderiv") &&
           load(&px_glGetShaderInfoLog, "glGetShaderInfoLog") &&
           load(&px_glDeleteShader, "glDeleteShader") &&
           load(&px_glCreateProgram, "glCreateProgram") &&
           load(&px_glAttachShader, "glAttachShader") &&
           load(&px_glBindAttribLocation, "glBindAttribLocation") &&
           load(&px_glLinkProgram, "glLinkProgram") &&
           load(&px_glGetProgramiv, "glGetProgramiv") &&
           load(&px_glGetProgramInfoLog, "glGetProgramInfoLog") &&
           load(&px_glDeleteProgram, "glDeleteProgram") &&
           load(&px_glUseProgram, "glUseProgram") &&
           load(&px_glGetUniformLocation, "glGetUniformLocation") &&
           load(&px_glUniform2f, "glUniform2f") && load(&px_glUniform1f, "glUniform1f") &&
           load(&px_glUniform1i, "glUniform1i") &&
           load(&px_glBlendFuncSeparate, "glBlendFuncSeparate") &&
           load(&px_glActiveTexture, "glActiveTexture") &&
           load(&px_glGenVertexArrays, "glGenVertexArrays") &&
           load(&px_glBindVertexArray, "glBindVertexArray") &&
           load(&px_glGenBuffers, "glGenBuffers") && load(&px_glBindBuffer, "glBindBuffer") &&
           load(&px_glBufferData, "glBufferData") &&
           load(&px_glBufferSubData, "glBufferSubData") && load(&px_glTexBuffer, "glTexBuffer") &&
           load(&px_glDrawArraysInstanced, "glDrawArraysInstanced") &&
           load(&px_glEnableVertexAttribArray, "glEnableVertexAttribArray") &&
           load(&px_glVertexAttribPointer, "glVertexAttribPointer");
}

bool driver_supports_gl_41(const GLubyte* version) {
    int major = 0;
    int minor = 0;
    return version &&
           std::sscanf(reinterpret_cast<const char*>(version), "%d.%d", &major, &minor) == 2 &&
           (major > 4 || (major == 4 && minor >= 1));
}

}  // namespace

bool px_gl_has_shaders() { return g_has_shaders; }

bool px_win_gl_create(px_window_t* window) {
    if (!window || !window->hwnd) {
        return false;
    }

    if (g_gl_unusable) {
        return false;
    }

    window->hdc = GetDC(window->hwnd);
    if (!window->hdc) {
        return false;
    }

    PIXELFORMATDESCRIPTOR pfd = describe_pixel_format();
    const int format = ChoosePixelFormat(window->hdc, &pfd);
    if (format == 0 || !SetPixelFormat(window->hdc, format, &pfd)) {
        std::println(stderr, "px: could not set a pixel format (error {})", GetLastError());
        return false;
    }
    PIXELFORMATDESCRIPTOR actual = {};
    actual.nSize = sizeof(actual);
    actual.nVersion = 1;
    if (DescribePixelFormat(window->hdc, format, sizeof(actual), &actual) != 0) {
        window->has_stencil = actual.cStencilBits >= 1;
    }

    if (!g_shared_context) {
        // Plain wglCreateContext: whatever the driver gives back is judged below, not requested up
        // front.
        g_shared_context = wglCreateContext(window->hdc);
        if (!g_shared_context) {
            std::println(stderr, "px: wglCreateContext failed (error {})", GetLastError());
            return false;
        }
        wglMakeCurrent(window->hdc, g_shared_context);

        const GLubyte* version = glGetString(GL_VERSION);
        if (!driver_supports_gl_41(version)) {
            std::println(stderr,
                         "px: driver does not support required OpenGL version 4.1 (got {})",
                         version ? reinterpret_cast<const char*>(version) : "?");
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g_shared_context);
            g_shared_context = nullptr;
            g_gl_unusable = true;
            return false;
        }

        const GLubyte* renderer = glGetString(GL_RENDERER);
        std::println(
            stderr, "px: GL {}, renderer {}, stencil={}", reinterpret_cast<const char*>(version),
            renderer ? reinterpret_cast<const char*>(renderer) : "?", window->has_stencil ? 1 : 0);

        g_has_shaders = load_modern_gl();
        if (!g_has_shaders) {
            // Drop GL entirely rather than painting through half-resolved entry points, which is
            // also what ST does when its loader fails.
            std::println(stderr, "px: failed to load OpenGL functions");
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g_shared_context);
            g_shared_context = nullptr;
            g_gl_unusable = true;
            return false;
        }
    }

    window->hglrc = g_shared_context;
    return true;
}

void px_win_gl_make_current(px_window_t* window) {
    if (window && window->hdc && window->hglrc) {
        wglMakeCurrent(window->hdc, window->hglrc);
    }
}

void px_win_gl_destroy(px_window_t* window) {
    if (!window) {
        return;
    }
    if (window->hdc) {
        if (wglGetCurrentDC() == window->hdc) {
            wglMakeCurrent(nullptr, nullptr);
        }
        // The window may already be gone: px_destroy_window normally runs after WM_DESTROY.
        if (window->hwnd && IsWindow(window->hwnd)) {
            ReleaseDC(window->hwnd, window->hdc);
        }
        window->hdc = nullptr;
    }
    // The context is process-wide and outlives individual windows.
    window->hglrc = nullptr;
}
