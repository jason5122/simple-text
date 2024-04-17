#include "main_window.h"
#include "ui/app/modifier_key.h"
#include <string>
#include <windowsx.h>
#include <winuser.h>

#include <iostream>

static app::Key GetKey(WPARAM vk) {
    static const struct {
        WPARAM fVK;
        app::Key fKey;
    } gPair[] = {
        {'A', app::Key::kA},
        {'B', app::Key::kB},
        {'C', app::Key::kC},
        // TODO: Implement the rest.
        {'N', app::Key::kN},
        {'Q', app::Key::kQ},
        {'W', app::Key::kW},
    };
    for (size_t i = 0; i < std::size(gPair); i++) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }
    return app::Key::kNone;
}

static app::ModifierKey GetModifierKeys(void) {
    app::ModifierKey modifiers = app::ModifierKey::kNone;
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        modifiers |= app::ModifierKey::kShift;
    }
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        modifiers |= app::ModifierKey::kControl;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
        modifiers |= app::ModifierKey::kAlt;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
        modifiers |= app::ModifierKey::kSuper;
    }
    return modifiers;
}

LRESULT MainWindow::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    case WM_CREATE: {
        m_hdc = GetDC(m_hwnd);

        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cDepthBits = 24,
            .cStencilBits = 0,
            .cAuxBuffers = 0,
        };

        int pixelformat = ChoosePixelFormat(m_hdc, &pfd);
        SetPixelFormat(m_hdc, pixelformat, &pfd);

        wglMakeCurrent(m_hdc, dummy_context.m_context);

        RECT rect = {0};
        GetClientRect(m_hwnd, &rect);

        int scaled_width = rect.right;
        int scaled_height = rect.bottom;

        app_window.onOpenGLActivate(scaled_width, scaled_height);

        return 0;
    }

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        wglMakeCurrent(m_hdc, dummy_context.m_context);

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        app_window.onDraw();

        SwapBuffers(m_hdc);

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        wglMakeCurrent(m_hdc, dummy_context.m_context);

        int width = (int)(short)LOWORD(lParam);
        int height = (int)(short)HIWORD(lParam);

        app_window.onResize(width, height);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        float dy = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        // Invert vertical scrolling.
        dy *= -1;

        static const unsigned long kDefaultScrollLinesPerWheelDelta = 3;
        unsigned long scroll_lines = kDefaultScrollLinesPerWheelDelta;
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scroll_lines, 0);
        dy *= scroll_lines;

        static const float kScrollbarPixelsPerLine = 100.0f / 3.0f;
        dy *= kScrollbarPixelsPerLine;

        app_window.onScroll(0, dy);
        return 0;
    }

    case WM_MOUSEHWHEEL: {
        float dx = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

        static const unsigned long kDefaultScrollCharsPerWheelDelta = 1;
        unsigned long scroll_chars = kDefaultScrollCharsPerWheelDelta;
        SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &scroll_chars, 0);
        dx *= scroll_chars;

        static const float kScrollbarPixelsPerLine = 100.0f / 3.0f;
        dx *= kScrollbarPixelsPerLine;

        app_window.onScroll(dx, 0);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(m_hwnd);

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);

        app_window.onLeftMouseDown(mouse_x, mouse_y);
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
        }
        return 0;
    }

    case WM_KEYDOWN: {
        app::Key key = GetKey(wParam);
        app::ModifierKey modifiers = GetModifierKeys();

        app_window.onKeyDown(key, modifiers);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    default:
        return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = NULL;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

        pThis->m_hwnd = hwnd;
    } else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    if (pThis) {
        return pThis->handleMessage(uMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void MainWindow::redraw() {
    InvalidateRect(m_hwnd, NULL, FALSE);
}

BOOL MainWindow::create(PCWSTR lpWindowName, DWORD dwStyle, int wid) {
    std::wstring class_name = L"ClassName";
    class_name += std::to_wstring(wid);

    WNDCLASS wc{
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(NULL),
        .hCursor = LoadCursor(NULL, IDC_IBEAM),
        // TODO: Change this color based on the editor background color.
        .hbrBackground = CreateSolidBrush(RGB(253, 253, 253)),
        .lpszClassName = &class_name[0],
    };

    RegisterClass(&wc);

    m_hwnd = CreateWindowEx(0, &class_name[0], lpWindowName, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(NULL), this);

    return (m_hwnd ? TRUE : FALSE);
}

BOOL MainWindow::destroy() {
    return DestroyWindow(m_hwnd);
}

void MainWindow::quit() {
    PostQuitMessage(0);
}
