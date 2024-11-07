#include "app/app_action.h"
#include "app/modifier_key.h"
#include "app/win32/resources.h"
#include "base/windows/unicode.h"
#include "main_window.h"
#include "util/escape_special_chars.h"
#include <shellscalingapi.h>
#include <shtypes.h>
#include <string>
#include <windowsx.h>
#include <wingdi.h>
#include <winuser.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace app {
namespace {

constexpr Key KeyFromKeyCode(WPARAM vk);
inline ModifierKey ModifierFromState();
inline void AddMenu(HWND hwnd);  // TODO: Clean this up.

}  // namespace
}  // namespace app

namespace app {

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
        // TODO: For debugging; remove this.
        // PostQuitMessage(0);

        wglMakeCurrent(m_hdc, dummy_context.m_context);

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        RECT rect{};
        GetClientRect(m_hwnd, &rect);
        int scaled_width = rect.right;
        int scaled_height = rect.bottom;

        app_window.onDraw(scaled_width, scaled_height);

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

        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(m_hwnd, &pt);
        int mouse_x = pt.x;
        int mouse_y = pt.y;

        app_window.onScroll(mouse_x, mouse_y, 0, std::round(dy));
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

        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(m_hwnd, &pt);
        int mouse_x = pt.x;
        int mouse_y = pt.y;

        app_window.onScroll(mouse_x, mouse_y, std::round(dx), 0);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(m_hwnd);

        // https://devblogs.microsoft.com/oldnewthing/20041018-00/?p=37543
        LONG click_time = GetMessageTime();
        UINT delta = click_time - last_click_time;
        if (delta > GetDoubleClickTime()) {
            click_count = 0;
        }
        // TODO: Prevent this from overflowing! This is unlikely, but prevent it anyways.
        ++click_count;
        last_click_time = click_time;

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);
        ModifierKey modifiers = ModifierFromState();
        ClickType click_type = ClickTypeFromCount(click_count);
        app_window.onLeftMouseDown(mouse_x, mouse_y, modifiers, click_type);
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
            ModifierKey modifiers = ModifierFromState();
            ClickType click_type = ClickTypeFromCount(click_count);
            app_window.onLeftMouseDrag(mouse_x, mouse_y, modifiers, click_type);
        }
        return 0;
    }

    case WM_KEYDOWN: {
        std::println("WM_KEYDOWN: {}", wParam);

        Key key = KeyFromKeyCode(wParam);
        ModifierKey modifiers = ModifierFromState();

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
            std::println("ImmersiveColorSet");
        } else {
            std::println("WM_SETTINGCHANGE, but theme was not changed.");
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_FILE_NEW_FILE:
            app_window.onAppAction(AppAction::kNewFile);
            break;
        case ID_FILE_NEW_WINDOW:
            app_window.onAppAction(AppAction::kNewWindow);
            break;
        case ID_FILE_EXIT:
            PostQuitMessage(0);
            break;
        }
        return 0;

    case WM_CHAR: {
        // https://github.com/libsdl-org/SDL/blob/b1c7b2f44f4aa9c80700911703e40795a8de3fcb/src/video/windows/SDL_windowsevents.c#L1217
        // https://discourse.libsdl.org/t/sdl-win32-simplify-unicode-text-input-code/47582
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
            std::println("WM_CHAR: {}", EscapeSpecialChars(str8));
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

BOOL MainWindow::create(PCWSTR lpWindowName, DWORD dwStyle, int wid) {
    std::wstring class_name = L"ClassName";
    class_name += std::to_wstring(wid);

    WNDCLASS wc{
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(NULL),
        .hCursor = LoadCursor(NULL, IDC_IBEAM),
        // TODO: Change this color based on the editor background color.
        .hbrBackground = CreateSolidBrush(RGB(253, 253, 253)),
        .lpszClassName = class_name.data(),
    };

    RegisterClass(&wc);

    m_hwnd =
        CreateWindowEx(0, class_name.data(), lpWindowName, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
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
    // std::println("scale_factor: {}", scale_factor);
    // TODO: Don't hard code this.
    return 1;
}

void MainWindow::setTitle(const std::string& title) {
    std::wstring str16 = base::windows::ConvertToUTF16(title);
    SetWindowText(m_hwnd, str16.data());
}

namespace {

constexpr Key KeyFromKeyCode(WPARAM vk) {
    constexpr struct {
        WPARAM vk;
        Key key;
    } kKeyMap[] = {
        {'A', Key::kA},
        {'B', Key::kB},
        {'C', Key::kC},
        {'D', Key::kD},
        {'E', Key::kE},
        {'F', Key::kF},
        {'G', Key::kG},
        {'H', Key::kH},
        {'I', Key::kI},
        {'J', Key::kJ},
        {'K', Key::kK},
        {'L', Key::kL},
        {'M', Key::kM},
        {'N', Key::kN},
        {'O', Key::kO},
        {'P', Key::kP},
        {'Q', Key::kQ},
        {'R', Key::kR},
        {'S', Key::kS},
        {'T', Key::kT},
        {'U', Key::kU},
        {'V', Key::kV},
        {'W', Key::kW},
        {'X', Key::kX},
        {'Y', Key::kY},
        {'Z', Key::kZ},
        {'0', Key::k0},
        {'1', Key::k1},
        {'2', Key::k2},
        {'3', Key::k3},
        {'4', Key::k4},
        {'5', Key::k5},
        {'6', Key::k6},
        {'7', Key::k7},
        {'8', Key::k8},
        {'9', Key::k9},
        {VK_RETURN, Key::kEnter},
        {VK_BACK, Key::kBackspace},
        {VK_TAB, Key::kTab},
        {VK_LEFT, Key::kLeftArrow},
        {VK_RIGHT, Key::kRightArrow},
        {VK_DOWN, Key::kDownArrow},
        {VK_UP, Key::kUpArrow},
    };
    for (size_t i = 0; i < std::size(kKeyMap); ++i) {
        if (kKeyMap[i].vk == vk) {
            return kKeyMap[i].key;
        }
    }
    return Key::kNone;
}

inline ModifierKey ModifierFromState() {
    ModifierKey modifiers = ModifierKey::kNone;
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        modifiers |= ModifierKey::kShift;
    }
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        modifiers |= ModifierKey::kControl;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
        modifiers |= ModifierKey::kAlt;
    }
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) {
        modifiers |= ModifierKey::kSuper;
    }
    return modifiers;
}

inline void AddMenu(HWND hwnd) {
    HMENU menubar = CreateMenu();
    HMENU file_menu = CreateMenu();

    // https://learn.microsoft.com/en-us/windows/win32/menurc/about-menus#menu-shortcut-keys
    AppendMenu(file_menu, MF_STRING, ID_FILE_NEW_FILE, L"New File\tCtrl+N");
    AppendMenu(file_menu, MF_STRING, ID_FILE_NEW_WINDOW, L"New Window\tCtrl+Shift+N");
    AppendMenu(file_menu, MF_STRING, ID_FILE_EXIT, L"E&xit\tCtrl+Q");
    AppendMenu(menubar, MF_POPUP, (UINT_PTR)file_menu, L"&File");

    SetMenu(hwnd, menubar);
}

}  // namespace

}  // namespace app
