#include "main_window.h"
#include <iostream>
#include <windowsx.h>

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

        wglSwapIntervalEXT(0);

        RECT rect = {0};
        GetClientRect(hwnd, &rect);

        int scaled_width = rect.right;
        int scaled_height = rect.bottom;

        app_window.onOpenGLActivate(scaled_width, scaled_height);

        return 0;
    }

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        wglMakeCurrent(ghDC, ghRC);

        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        app_window.onDraw();

        SwapBuffers(ghDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        wglMakeCurrent(ghDC, ghRC);

        int width = (int)(short)LOWORD(lParam);
        int height = (int)(short)HIWORD(lParam);

        app_window.onResize(width, height);

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        float dy = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        // Invert vertical scrolling.
        dy *= -1;

        // TODO: Replace this magic number.
        dy *= 60;

        app_window.onScroll(0, dy);

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_MOUSEHWHEEL: {
        float dx = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));

        app_window.onScroll(dx, 0);

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);

        app_window.onLeftMouseDown(mouse_x, mouse_y);

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (wParam == MK_LBUTTON) {
            int mouse_x = GET_X_LPARAM(lParam);
            int mouse_y = GET_Y_LPARAM(lParam);

            app_window.onLeftMouseDrag(mouse_x, mouse_y);

            InvalidateRect(hwnd, NULL, FALSE);
        }
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

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = NULL;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

        pThis->hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (pThis) {
        return pThis->handleMessage(uMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

BOOL MainWindow::create(PCWSTR lpWindowName, DWORD dwStyle) {
    WNDCLASS wc{
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(NULL),
        // TODO: Change this color based on the editor background color.
        .hbrBackground = CreateSolidBrush(RGB(253, 253, 253)),
        .lpszClassName = className(),
    };

    RegisterClass(&wc);

    hwnd = CreateWindowEx(0, className(), lpWindowName, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(NULL), this);

    return (hwnd ? TRUE : FALSE);
}
