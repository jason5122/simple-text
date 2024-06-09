#include "dummy_context.h"
#include "renderer/opengl_functions.h"
#include "util/profile_util.h"
#include <glad/glad_wgl.h>
#include <iostream>

void DummyContext::initialize() {
    m_hwnd = CreateWindowEx(0, L"DummyClass", L"Dummy", WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(NULL), this);

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

    int pixelformat;
    {
        PROFILE_BLOCK("ChoosePixelFormat() + SetPixelFormat()");
        pixelformat = ChoosePixelFormat(m_hdc, &pfd);
        SetPixelFormat(m_hdc, pixelformat, &pfd);
    }

    m_context = wglCreateContext(m_hdc);

    wglMakeCurrent(m_hdc, m_context);

    if (!gladLoadGL() || !gladLoadWGL(m_hdc)) {
        std::cerr << "Failed to initialize GLAD\n";
    }

    wglSwapIntervalEXT(0);
}
