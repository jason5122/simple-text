#include "win32_window.h"

#include <shellscalingapi.h>
#include <shtypes.h>
#include <string>
#include <windowsx.h>
#include <wingdi.h>
#include <winuser.h>

#include "base/windows/unicode.h"
#include "gui/platform/key.h"
#include "gui/platform/win32/resources.h"

// TODO: Debug use; remove this.
#include "util/escape_special_chars.h"
#include <fmt/base.h>

namespace gui {

namespace {

constexpr Key KeyFromKeyCode(WPARAM vk);
inline ModifierKey ModifierFromState();
inline void AddMenu(HWND hwnd);  // TODO: Clean this up.

}  // namespace

LRESULT Win32Window::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

        // TODO: See if we need to set the size here.
        // RECT rect;
        // GetClientRect(m_hwnd, &rect);
        // int scaled_width = rect.right;
        // int scaled_height = rect.bottom;
        // app_window.setSize({scaled_width, scaled_height});
        app_window.on_opengl_activate();

        return 0;
    }

    case WM_PAINT:
    case WM_DISPLAYCHANGE: {
        // TODO: For debugging; remove this.
        // PostQuitMessage(0);

        wglMakeCurrent(m_hdc, dummy_context.m_context);

        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);

        // TODO: See if we need this.
        // RECT rect;
        // GetClientRect(m_hwnd, &rect);
        // int scaled_width = rect.right;
        // int scaled_height = rect.bottom;

        app_window.draw();

        SwapBuffers(m_hdc);

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_SIZE: {
        wglMakeCurrent(m_hdc, dummy_context.m_context);

        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        app_window.set_size({width, height});
        app_window.layout();
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
        Point mouse_pos = {mouse_x, mouse_y};
        Delta delta = {0, static_cast<int>(std::round(dy))};

        app_window.perform_scroll(mouse_pos, delta);
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
        Point mouse_pos = {mouse_x, mouse_y};
        Delta delta = {static_cast<int>(std::round(dx)), 0};

        app_window.perform_scroll(mouse_pos, delta);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(m_hwnd);

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);

        // https://devblogs.microsoft.com/oldnewthing/20041018-00/?p=37543
        LONG click_time = GetMessageTime();
        UINT delta_time = click_time - last_click_time;
        int delta_mouse_x = std::abs(mouse_x - last_mouse_x);
        int delta_mouse_y = std::abs(mouse_y - last_mouse_y);

        // TODO: See if static is fine, or if we should cache them as members.
        static UINT double_click_interval = GetDoubleClickTime();
        static int double_click_x_tolerance = GetSystemMetrics(SM_CXDOUBLECLK);
        static int double_click_y_tolerance = GetSystemMetrics(SM_CYDOUBLECLK);

        if (delta_time > double_click_interval || delta_mouse_x > double_click_x_tolerance ||
            delta_mouse_y > double_click_y_tolerance) {
            click_count = 0;
        }
        ++click_count;
        last_click_time = click_time;
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;

        Point mouse_pos = {mouse_x, mouse_y};
        ModifierKey modifiers = ModifierFromState();
        ClickType click_type = click_type_from_count(click_count);
        app_window.left_mouse_down(mouse_pos, modifiers, click_type);
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();

        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);
        Point mouse_pos = {mouse_x, mouse_y};
        app_window.left_mouse_up(mouse_pos);
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mouse_x = GET_X_LPARAM(lParam);
        int mouse_y = GET_Y_LPARAM(lParam);
        Point mouse_pos = {mouse_x, mouse_y};

        if (wParam & MK_LBUTTON) {
            ModifierKey modifiers = ModifierFromState();
            ClickType click_type = click_type_from_count(click_count);
            app_window.left_mouse_drag(mouse_pos, modifiers, click_type);
        } else {
            if (!tracking_mouse) {
                TRACKMOUSEEVENT tme = {
                    .cbSize = sizeof(TRACKMOUSEEVENT),
                    .dwFlags = TME_LEAVE,
                    .hwndTrack = m_hwnd,
                };
                tracking_mouse = TrackMouseEvent(&tme);
            }
            app_window.mouse_position_changed(mouse_pos);
        }
        return 0;
    }

    // Microsoft-recommended implementation: https://stackoverflow.com/a/68029657/14698275
    case WM_MOUSELEAVE: {
        tracking_mouse = false;
        app_window.mouse_position_changed(std::nullopt);
        return 0;
    }

    case WM_KEYDOWN: {
        fmt::println("WM_KEYDOWN: {}", wParam);

        Key key = KeyFromKeyCode(wParam);
        ModifierKey modifiers = ModifierFromState();

        bool handled = app_window.on_key_down(key, modifiers);
        bool can_be_char = CanBeChar(key) && modifiers == ModifierKey::kNone;

        // Remove translated WM_CHAR if we've already handled the keybind or if it is not
        // representable as a char.
        if (handled || !can_be_char) {
            MSG msg;
            PeekMessage(&msg, m_hwnd, WM_CHAR, WM_CHAR, PM_REMOVE);
        }

        return 0;
    }

    case WM_DESTROY: {
        app_window.on_close();
        return 0;
    }

    // TODO: Test light/dark mode switching on an activated Windows license.
    case WM_SETTINGCHANGE: {
        if (!lstrcmp(LPCTSTR(lParam), L"ImmersiveColorSet")) {
            fmt::println("ImmersiveColorSet");
        } else {
            fmt::println("WM_SETTINGCHANGE, but theme was not changed.");
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_FILE_NEW_FILE:
            app_window.on_app_action(AppAction::kNewFile);
            break;
        case ID_FILE_NEW_WINDOW:
            app_window.on_app_action(AppAction::kNewWindow);
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
                utf16[1] = static_cast<WCHAR>(wParam);
            } else {
                utf16[0] = static_cast<WCHAR>(wParam);
            }

            std::string str8 = base::windows::ConvertToUTF8(utf16);
            fmt::println("WM_CHAR: {}", util::escape_special_chars(str8));
            app_window.on_insert_text(str8);

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

namespace {
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Win32Window* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<Win32Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

        pThis->set_hwnd(hwnd);
    } else {
        pThis = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) {
        return pThis->handle_message(uMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
}  // namespace

void Win32Window::redraw() { InvalidateRect(m_hwnd, nullptr, false); }

BOOL Win32Window::create(PCWSTR lpWindowName, DWORD dwStyle, int wid) {
    // TODO: Clean this up. Do we even need a unique class name?
    std::wstring class_name = L"ClassName";
    class_name += std::to_wstring(wid);

    WNDCLASSEX wc = {
        .cbSize = sizeof(WNDCLASSEX),
        .lpfnWndProc = WindowProc,
        .hInstance = GetModuleHandle(nullptr),
        // We set our own cursor shapes.
        .hCursor = nullptr,
        // We paint our own backgrounds.
        .hbrBackground = nullptr,
        .lpszClassName = class_name.data(),
    };
    RegisterClassEx(&wc);

    // Set cursor to arrow (otherwise IDC_WAIT appears as a default).
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    m_hwnd =
        CreateWindowEx(0, class_name.data(), lpWindowName, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                       CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(nullptr), this);

    AddMenu(m_hwnd);

    return m_hwnd ? true : false;
}

BOOL Win32Window::destroy() { return DestroyWindow(m_hwnd); }

void Win32Window::quit() { PostQuitMessage(0); }

int Win32Window::width() {
    RECT size;
    GetClientRect(m_hwnd, &size);
    return size.right;
}

int Win32Window::height() {
    RECT size;
    GetClientRect(m_hwnd, &size);
    return size.bottom;
}

int Win32Window::scale() {
    HMONITOR hmon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTOPRIMARY);
    DEVICE_SCALE_FACTOR scale_factor;
    GetScaleFactorForMonitor(hmon, &scale_factor);
    fmt::println("Win32 window scale: {}", static_cast<int>(scale_factor));
    // TODO: Don't hard code this.
    return 1;
}

void Win32Window::set_title(std::string_view title) {
    std::wstring str16 = base::windows::ConvertToUTF16(title);
    SetWindowText(m_hwnd, str16.data());
}

HWND Win32Window::get_hwnd() const { return m_hwnd; }

void Win32Window::set_hwnd(HWND hwnd) { m_hwnd = hwnd; }

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
        {VK_OEM_PLUS, Key::kEqual},
        {VK_OEM_MINUS, Key::kMinus},
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
    AppendMenu(menubar, MF_POPUP, reinterpret_cast<UINT_PTR>(file_menu), L"&File");

    SetMenu(hwnd, menubar);
}

}  // namespace

}  // namespace gui
