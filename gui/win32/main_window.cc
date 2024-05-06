#include "gui/modifier_key.h"
#include "main_window.h"
#include <shellscalingapi.h>
#include <shtypes.h>
#include <string>
#include <windowsx.h>
#include <winuser.h>

#include <iostream>

static app::Key GetKey(WPARAM vk) {
    static constexpr struct {
        WPARAM fVK;
        app::Key fKey;
    } gPair[] = {
        {'A', app::Key::kA},
        {'B', app::Key::kB},
        {'C', app::Key::kC},
        {'D', app::Key::kD},
        {'E', app::Key::kE},
        {'F', app::Key::kF},
        {'G', app::Key::kG},
        {'H', app::Key::kH},
        {'I', app::Key::kI},
        {'J', app::Key::kJ},
        {'K', app::Key::kK},
        {'L', app::Key::kL},
        {'M', app::Key::kM},
        {'N', app::Key::kN},
        {'O', app::Key::kO},
        {'P', app::Key::kP},
        {'Q', app::Key::kQ},
        {'R', app::Key::kR},
        {'S', app::Key::kS},
        {'T', app::Key::kT},
        {'U', app::Key::kU},
        {'V', app::Key::kV},
        {'W', app::Key::kW},
        {'X', app::Key::kX},
        {'Y', app::Key::kY},
        {'Z', app::Key::kZ},
        {'0', app::Key::k0},
        {'1', app::Key::k1},
        {'2', app::Key::k2},
        {'3', app::Key::k3},
        {'4', app::Key::k4},
        {'5', app::Key::k5},
        {'6', app::Key::k6},
        {'7', app::Key::k7},
        {'8', app::Key::k8},
        {'9', app::Key::k9},
        {VK_BACK, app::Key::kBackspace},
        {VK_LEFT, app::Key::kLeftArrow},
        {VK_RIGHT, app::Key::kRightArrow},
        {VK_DOWN, app::Key::kDownArrow},
        {VK_UP, app::Key::kUpArrow},
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

        static constexpr unsigned long kDefaultScrollLinesPerWheelDelta = 3;
        unsigned long scroll_lines = kDefaultScrollLinesPerWheelDelta;
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scroll_lines, 0);
        dy *= scroll_lines;

        static constexpr float kScrollbarPixelsPerLine = 100.0f / 3.0f;
        dy *= kScrollbarPixelsPerLine;

        app_window.onScroll(0, dy);
        return 0;
    }

    case WM_MOUSEHWHEEL: {
        float dx = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;

        static constexpr unsigned long kDefaultScrollCharsPerWheelDelta = 1;
        unsigned long scroll_chars = kDefaultScrollCharsPerWheelDelta;
        SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &scroll_chars, 0);
        dx *= scroll_chars;

        static constexpr float kScrollbarPixelsPerLine = 100.0f / 3.0f;
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

    case WM_DESTROY: {
        app_window.onClose();
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

int MainWindow::width() {
    RECT size;
    GetClientRect(m_hwnd, &size);
    return size.right;
}

int MainWindow::height() {
    RECT size;
    GetClientRect(m_hwnd, &size);
    return size.bottom;
}

int MainWindow::scaleFactor() {
    HMONITOR hmon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY);
    DEVICE_SCALE_FACTOR scale_factor;
    GetScaleFactorForMonitor(hmon, &scale_factor);
    std::cerr << "scale_factor: " << scale_factor << '\n';
    // TODO: Don't hard code this.
    return 1;
}
