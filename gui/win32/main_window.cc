#include "base/windows/unicode.h"
#include "gui/gui_action.h"
#include "gui/modifier_key.h"
#include "main_window.h"
#include "util/escape_special_chars.h"
#include "util/profile_util.h"
#include <shellscalingapi.h>
#include <shtypes.h>
#include <string>
#include <windowsx.h>
#include <winuser.h>

#include <format>
#include <iostream>

#define ID_FILE_NEW_FILE 1
#define ID_FILE_NEW_WINDOW 2
#define ID_FILE_EXIT 3

namespace gui {

static inline gui::Key GetKey(WPARAM vk) {
    static constexpr struct {
        WPARAM fVK;
        gui::Key fKey;
    } gPair[] = {
        {'A', gui::Key::kA},
        {'B', gui::Key::kB},
        {'C', gui::Key::kC},
        {'D', gui::Key::kD},
        {'E', gui::Key::kE},
        {'F', gui::Key::kF},
        {'G', gui::Key::kG},
        {'H', gui::Key::kH},
        {'I', gui::Key::kI},
        {'J', gui::Key::kJ},
        {'K', gui::Key::kK},
        {'L', gui::Key::kL},
        {'M', gui::Key::kM},
        {'N', gui::Key::kN},
        {'O', gui::Key::kO},
        {'P', gui::Key::kP},
        {'Q', gui::Key::kQ},
        {'R', gui::Key::kR},
        {'S', gui::Key::kS},
        {'T', gui::Key::kT},
        {'U', gui::Key::kU},
        {'V', gui::Key::kV},
        {'W', gui::Key::kW},
        {'X', gui::Key::kX},
        {'Y', gui::Key::kY},
        {'Z', gui::Key::kZ},
        {'0', gui::Key::k0},
        {'1', gui::Key::k1},
        {'2', gui::Key::k2},
        {'3', gui::Key::k3},
        {'4', gui::Key::k4},
        {'5', gui::Key::k5},
        {'6', gui::Key::k6},
        {'7', gui::Key::k7},
        {'8', gui::Key::k8},
        {'9', gui::Key::k9},
        {VK_RETURN, gui::Key::kEnter},
        {VK_BACK, gui::Key::kBackspace},
        {VK_LEFT, gui::Key::kLeftArrow},
        {VK_RIGHT, gui::Key::kRightArrow},
        {VK_DOWN, gui::Key::kDownArrow},
        {VK_UP, gui::Key::kUpArrow},
    };
    for (size_t i = 0; i < std::size(gPair); i++) {
        if (gPair[i].fVK == vk) {
            return gPair[i].fKey;
        }
    }
    return gui::Key::kNone;
}

static inline gui::ModifierKey GetModifiers(void) {
    gui::ModifierKey modifiers = gui::ModifierKey::kNone;
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        modifiers |= gui::ModifierKey::kShift;
    }
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        modifiers |= gui::ModifierKey::kControl;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
        modifiers |= gui::ModifierKey::kAlt;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
        modifiers |= gui::ModifierKey::kSuper;
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

        RECT rect{};
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

        // TODO: For debugging; remove this.
        // app_window.stopLaunchTimer();

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

        app_window.onScroll(0, std::round(dy));
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

        app_window.onScroll(std::round(dx), 0);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(m_hwnd);

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);
        gui::ModifierKey modifiers = GetModifiers();

        app_window.onLeftMouseDown(mouse_x, mouse_y, modifiers);
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (wParam & MK_LBUTTON) {
            int mouse_x = GET_X_LPARAM(lParam);
            int mouse_y = GET_Y_LPARAM(lParam);
            gui::ModifierKey modifiers = GetModifiers();

            app_window.onLeftMouseDrag(mouse_x, mouse_y, modifiers);
        }
        return 0;
    }

    case WM_KEYDOWN: {
        std::cerr << std::format("WM_KEYDOWN: {}", wParam) << '\n';

        gui::Key key = GetKey(wParam);
        gui::ModifierKey modifiers = GetModifiers();

        bool handled = app_window.onKeyDown(key, modifiers);

        // Remove translated WM_CHAR if we've already handled the keybind.
        if (handled) {
            MSG msg;
            PeekMessage(&msg, m_hwnd, WM_CHAR, WM_CHAR, PM_REMOVE);
        }

        return 0;
    }

    case WM_DESTROY: {
        app_window.onClose();
        return 0;
    }

    // TODO: Test light/dark mode switching on an activated Windows license.
    case WM_SETTINGCHANGE: {
        if (!lstrcmp(LPCTSTR(lParam), L"ImmersiveColorSet")) {
            std::cerr << "ImmersiveColorSet\n";
        } else {
            std::cerr << "WM_SETTINGCHANGE, but theme was not changed.\n";
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_FILE_NEW_FILE:
            app_window.onGuiAction(gui::GuiAction::kNewFile);
            break;
        case ID_FILE_NEW_WINDOW:
            app_window.onGuiAction(gui::GuiAction::kNewWindow);
            break;
        case ID_FILE_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_CHAR: {
        if (IS_HIGH_SURROGATE(wParam)) {
            high_surrogate = wParam;
        } else {
            WCHAR utf16[3]{};  // Initialized to {'\0', '\0', '\0'}.

            if (high_surrogate) {
                utf16[0] = high_surrogate;
                utf16[1] = (WCHAR)wParam;
            } else {
                utf16[0] = (WCHAR)wParam;
            }

            std::string str8 = base::windows::ConvertToUTF8(utf16);
            std::cerr << std::format("WM_CHAR: {}", EscapeSpecialChars(str8)) << '\n';
            app_window.onInsertText(str8);

            high_surrogate = '\0';
        }
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

static inline void AddMenu(HWND hwnd) {
    HMENU menubar = CreateMenu();
    HMENU file_menu = CreateMenu();

    AppendMenu(file_menu, MF_STRING, ID_FILE_NEW_FILE, L"New File");
    AppendMenu(file_menu, MF_STRING, ID_FILE_NEW_WINDOW, L"New Window");
    AppendMenu(file_menu, MF_STRING, ID_FILE_EXIT, L"Exit");
    AppendMenu(menubar, MF_POPUP, (UINT_PTR)file_menu, L"File");

    SetMenu(hwnd, menubar);
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

    AddMenu(m_hwnd);

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
    // std::cerr << "scale_factor: " << scale_factor << '\n';
    // TODO: Don't hard code this.
    return 1;
}

void MainWindow::setTitle(const std::string& title) {
    SetWindowText(m_hwnd, L"untitleddddd");
}

}
