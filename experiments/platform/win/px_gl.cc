// WGL context creation and the modern-GL entry point loader.
//
// Two decisions copied from ST's binary:
//
//   * The pixel format is SINGLE buffered. sublime_text.exe imports neither SwapBuffers (gdi32)
//     nor wglSwapLayerBuffers, and carries no such string; it imports glFlush and finishes frames
//     with that. A single-buffered drawable persists between frames, which is precisely what makes
//     repainting only the invalid rects correct -- the same property kCGLPFABackingStore buys on
//     macOS. Both backends are dirty-rect renderers for the same underlying reason.
//
//   * Modern GL is resolved at runtime. The import table holds 23 GL 1.1 entry points and the
//     binary carries the strings "OPENGL32.dll" and "wglGetProcAddress"; everything past 1.1 is
//     looked up rather than linked.
//
// One departure: ST imports wglShareLists, implying a context per window sharing one object
// namespace. This creates a single context and makes it current against each window's HDC, which
// is legal because every window is given the same pixel format, and is closer to what the macOS
// side does (one process-wide CGLContextObj).

#include "experiments/platform/px_gl.h"
#include "experiments/platform/win/px_win_private.h"

#include <print>

// Defined here, declared in px_gl.h.
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
PFN_glUseProgram px_glUseProgram = nullptr;
PFN_glGetUniformLocation px_glGetUniformLocation = nullptr;
PFN_glUniform2f px_glUniform2f = nullptr;
PFN_glUniform1i px_glUniform1i = nullptr;
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

// WGL_ARB_create_context, spelled out so this does not need wglext.h.
constexpr int kWglContextMajorVersionArb = 0x2091;
constexpr int kWglContextMinorVersionArb = 0x2092;
constexpr int kWglContextProfileMaskArb = 0x9126;
constexpr int kWglContextCoreProfileBitArb = 0x00000001;

using PFN_wglCreateContextAttribsARB = HGLRC(WINAPI*)(HDC, HGLRC, const int*);

HGLRC g_shared_context = nullptr;
PFN_wglCreateContextAttribsARB g_create_context_attribs = nullptr;
bool g_probed = false;
bool g_has_shaders = false;

PIXELFORMATDESCRIPTOR describe_pixel_format() {
  PIXELFORMATDESCRIPTOR pfd = {};
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  // No PFD_DOUBLEBUFFER: see the file comment.
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cAlphaBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;
  return pfd;
}

void* resolve(const char* name) {
  if (void* p = reinterpret_cast<void*>(wglGetProcAddress(name))) {
    return p;
  }
  // wglGetProcAddress returns null for GL 1.1 entry points; those live in the DLL's export table.
  if (HMODULE gl = GetModuleHandleW(L"opengl32.dll")) {
    return reinterpret_cast<void*>(GetProcAddress(gl, name));
  }
  return nullptr;
}

template <typename T>
bool load(T* slot, const char* name) {
  *slot = reinterpret_cast<T>(resolve(name));
  return *slot != nullptr;
}

void load_modern_gl() {
  bool ok = true;
  ok &= load(&px_glCreateShader, "glCreateShader");
  ok &= load(&px_glShaderSource, "glShaderSource");
  ok &= load(&px_glCompileShader, "glCompileShader");
  ok &= load(&px_glGetShaderiv, "glGetShaderiv");
  ok &= load(&px_glGetShaderInfoLog, "glGetShaderInfoLog");
  ok &= load(&px_glDeleteShader, "glDeleteShader");
  ok &= load(&px_glCreateProgram, "glCreateProgram");
  ok &= load(&px_glAttachShader, "glAttachShader");
  ok &= load(&px_glBindAttribLocation, "glBindAttribLocation");
  ok &= load(&px_glLinkProgram, "glLinkProgram");
  ok &= load(&px_glUseProgram, "glUseProgram");
  ok &= load(&px_glGetUniformLocation, "glGetUniformLocation");
  ok &= load(&px_glUniform2f, "glUniform2f");
  ok &= load(&px_glUniform1i, "glUniform1i");
  ok &= load(&px_glActiveTexture, "glActiveTexture");
  ok &= load(&px_glGenVertexArrays, "glGenVertexArrays");
  ok &= load(&px_glBindVertexArray, "glBindVertexArray");
  ok &= load(&px_glGenBuffers, "glGenBuffers");
  ok &= load(&px_glBindBuffer, "glBindBuffer");
  ok &= load(&px_glBufferData, "glBufferData");
  ok &= load(&px_glBufferSubData, "glBufferSubData");
  ok &= load(&px_glTexBuffer, "glTexBuffer");
  ok &= load(&px_glDrawArraysInstanced, "glDrawArraysInstanced");
  ok &= load(&px_glEnableVertexAttribArray, "glEnableVertexAttribArray");
  ok &= load(&px_glVertexAttribPointer, "glVertexAttribPointer");
  g_has_shaders = ok;
}

// wglCreateContextAttribsARB can only be resolved from a context that already exists, so a
// throwaway window and legacy context go first.
void probe_wgl_extensions() {
  if (g_probed) {
    return;
  }
  g_probed = true;

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = DefWindowProcW;
  wc.lpszClassName = L"PX_WGL_PROBE";
  RegisterClassExW(&wc);

  HWND hwnd = CreateWindowExW(0, L"PX_WGL_PROBE", L"", WS_OVERLAPPED, 0, 0, 1, 1, nullptr, nullptr,
                              nullptr, nullptr);
  if (!hwnd) {
    return;
  }
  HDC hdc = GetDC(hwnd);
  PIXELFORMATDESCRIPTOR pfd = describe_pixel_format();
  const int format = ChoosePixelFormat(hdc, &pfd);
  if (format != 0 && SetPixelFormat(hdc, format, &pfd)) {
    if (HGLRC rc = wglCreateContext(hdc)) {
      wglMakeCurrent(hdc, rc);
      g_create_context_attribs =
          reinterpret_cast<PFN_wglCreateContextAttribsARB>(resolve("wglCreateContextAttribsARB"));
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(rc);
    }
  }
  ReleaseDC(hwnd, hdc);
  DestroyWindow(hwnd);
  UnregisterClassW(L"PX_WGL_PROBE", nullptr);
}

}  // namespace

bool px_gl_has_shaders() {
  return g_has_shaders;
}

bool px_win_gl_create(px_window_t* window) {
  if (!window || !window->hwnd) {
    return false;
  }

  probe_wgl_extensions();

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

  if (!g_shared_context) {
    if (g_create_context_attribs) {
      const int attribs[] = {
          kWglContextMajorVersionArb, 3,
          kWglContextMinorVersionArb, 2,
          kWglContextProfileMaskArb,  kWglContextCoreProfileBitArb,
          0,
      };
      g_shared_context = g_create_context_attribs(window->hdc, nullptr, attribs);
    }
    if (!g_shared_context) {
      // No WGL_ARB_create_context, or the driver refused 3.2 core. A legacy context still lets the
      // window come up and the event trace run; drawing degrades via px_gl_has_shaders().
      g_shared_context = wglCreateContext(window->hdc);
    }
    if (!g_shared_context) {
      std::println(stderr, "px: wglCreateContext failed (error {})", GetLastError());
      return false;
    }
    wglMakeCurrent(window->hdc, g_shared_context);
    load_modern_gl();

    const GLubyte* version = glGetString(GL_VERSION);
    std::println(stderr, "px: GL {}, shaders={}",
                 version ? reinterpret_cast<const char*>(version) : "?",
                 px_gl_has_shaders() ? 1 : 0);
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
