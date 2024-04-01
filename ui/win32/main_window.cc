#include "main_window.h"
#include "ui/win32/resource.h"
#include <glad/glad.h>
#include <iostream>
#include <wingdi.h>

BOOL bSetupPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cDepthBits = 24,
        .cStencilBits = 0,
        .cAuxBuffers = 0,
    };

    int pixelformat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelformat) {
        return false;
    }
    return SetPixelFormat(hdc, pixelformat, &pfd);
}

PCWSTR MainWindow::ClassName() const {
    return L"Sample Window Class";
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        ghDC = GetDC(m_hwnd);
        if (!bSetupPixelFormat(ghDC)) {
            PostQuitMessage(0);
        }
        ghRC = wglCreateContext(ghDC);
        wglMakeCurrent(ghDC, ghRC);

        if (!gladLoadGL()) {
            std::cerr << "Failed to initialize GLAD\n";
        }

        std::cerr << glGetString(GL_VERSION) << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(253 / 255.0, 253 / 255.0, 253 / 255.0, 1.0);

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        float scaled_width = rect.right;
        float scaled_height = rect.bottom;

        image_renderer.setup(scaled_width, scaled_height);
        rect_renderer.setup(scaled_width, scaled_height);

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        float line_height = 40;
        float scaled_width = rect.right;
        float scaled_height = rect.bottom;
        float scaled_editor_offset_x = 200 * 2;
        float scaled_editor_offset_y = 30 * 2;
        float scaled_status_bar_height = line_height;

        glClear(GL_COLOR_BUFFER_BIT);

        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        rect_renderer.resize(scaled_width, scaled_height);
        rect_renderer.draw(0, 0, 0, 0, line_height, 100, 500, scaled_editor_offset_x,
                           scaled_editor_offset_y, scaled_status_bar_height);

        // glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
        // image_renderer.resize(scaled_width, scaled_height);
        // image_renderer.draw(0, 0, 200, 30);

        SwapBuffers(ghDC);

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
        // TODO: Reference Direct2DClock for smooth resizing.
        // InvalidateRect(m_hwnd, NULL, FALSE);
        // RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE);
        RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        return 0;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_QUIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
    return true;
}
