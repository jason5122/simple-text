#include "main_window.h"

LRESULT MainWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        ghDC = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cDepthBits = 24,
            .cStencilBits = 0,
            .cAuxBuffers = 0,
        };

        int pixelformat = ChoosePixelFormat(ghDC, &pfd);
        SetPixelFormat(ghDC, pixelformat, &pfd);

        ghRC = wglCreateContext(ghDC);
        wglMakeCurrent(ghDC, ghRC);

        if (!gladLoadGL() || !gladLoadWGL(ghDC)) {
            std::cerr << "Failed to initialize GLAD\n";
        }

        std::cerr << glGetString(GL_VERSION) << '\n';

        wglSwapIntervalEXT(0);

        std::cerr << wglGetSwapIntervalEXT() << '\n';

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);

        glClearColor(1.0, 0.0, 0.0, 1.0);

        return 0;
    }
    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        std::cerr << "WM_PAINT\n";

        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        {
            PROFILE_BLOCK("draw");

            glClear(GL_COLOR_BUFFER_BIT);

            SwapBuffers(ghDC);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_QUIT:
            PostQuitMessage(0);
            break;
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
