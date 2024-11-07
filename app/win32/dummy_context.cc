#include "dummy_context.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"

void DummyContext::initialize() {
    m_hwnd = CreateWindowEx(0, L"DummyClass", L"Dummy", WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(nullptr), this);

    HDC m_hdc = GetDC(m_hwnd);

    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cDepthBits = 24,
        .cStencilBits = 0,
        .cAuxBuffers = 0,
    };

    PROFILE_BLOCK("ChoosePixelFormat() + SetPixelFormat()");
    int pixelformat = ChoosePixelFormat(m_hdc, &pfd);
    SetPixelFormat(m_hdc, pixelformat, &pfd);

    m_context = wglCreateContext(m_hdc);

    wglMakeCurrent(m_hdc, m_context);

    // TODO: Determine if this has any effect.
    // TODO: If we need this, load this function pointer manually.
    // wglSwapIntervalEXT(0);
}
