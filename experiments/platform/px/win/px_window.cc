// The Win32 window: PX_WINDOW_CLASS, its WndProc, and the window half of the flat px API.
//
// The WndProc mirrors the structure of ST's at 0x1401be53a. Every arm does the same three things
// the Cocoa entry points do: memset a px_event_t, fill the fields its tag needs, and call one
// funnel. The tag values are the ones recovered from ST's jump tables, so a trace from this binary
// and a trace from the macOS backend line up event for event.
//
// Message -> tag, as decoded:
//     WM_KEYDOWN/KEYUP/SYSKEY*   -> 0   (ST stores no tag at all; the memset already left 0)
//     WM_CHAR                    -> 1
//     WM_*BUTTONDOWN/UP/DBLCLK   -> 2
//     WM_MOUSEMOVE               -> 3
//     WM_MOUSELEAVE              -> 4
//     WM_HSCROLL/WM_VSCROLL      -> 6
//     WM_CAPTURECHANGED          -> 7
//     WM_SIZE                    -> 8
//     WM_DESTROY                 -> 9
//     WM_DPICHANGED              -> 10
//     WM_DROPFILES               -> 11
//     WM_SETFOCUS                -> 13
//     WM_KILLFOCUS               -> 14
//     WM_SETTINGCHANGE           -> 21

#include "experiments/platform/px/gl_render_context.h"
#include "experiments/platform/px/px_gl.h"
#include "experiments/platform/px/win/px_win_private.h"

#include <imm.h>
#include <shellapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <print>
#include <string>

namespace {

constexpr double kMinEventFlushInterval = 1.0 / 120.0;

// Drives animation_tick. The macOS backend gets this from a CVDisplayLink; ST's Windows side paces
// against the compositor with DwmGetCompositionTimingInfo / DwmFlush, which it imports. A plain
// timer is used here because it behaves predictably inside a VM, where DWM timing often does not.
constexpr UINT_PTR kAnimationTimerId = 1;
constexpr UINT kAnimationIntervalMs = 16;

std::vector<px_window_t*>& windows_storage() {
    static std::vector<px_window_t*> windows;
    return windows;
}

std::vector<std::function<void()>>& post_event_callbacks() {
    static std::vector<std::function<void()>> callbacks;
    return callbacks;
}

dummy_px_window_event_handler& dummy_handler() {
    static dummy_px_window_event_handler handler;
    return handler;
}

bool g_class_registered = false;

// Scratch for WM_DROPFILES, valid only for the duration of the dispatch.
struct PathList {
    std::vector<std::string> storage;
    std::vector<const char*> pointers;

    void rebuild_pointers() {
        pointers.clear();
        pointers.reserve(storage.size());
        for (const std::string& s : storage) {
            pointers.push_back(s.c_str());
        }
    }
    const char* const* data() const { return pointers.empty() ? nullptr : pointers.data(); }
    int count() const { return static_cast<int>(pointers.size()); }
};

PathList& drop_paths() {
    static PathList paths;
    return paths;
}

std::string to_utf8(const wchar_t* utf16, int length_in_units) {
    if (!utf16 || length_in_units == 0) {
        return {};
    }
    const int needed =
        WideCharToMultiByte(CP_UTF8, 0, utf16, length_in_units, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16, length_in_units, out.data(), needed, nullptr, nullptr);
    return out;
}

std::wstring to_utf16(const char* utf8) {
    if (!utf8 || !*utf8) {
        return {};
    }
    const int needed = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (needed <= 0) {
        return {};
    }
    std::wstring out(static_cast<size_t>(needed - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, out.data(), needed);
    return out;
}

HCURSOR win_cursor(px_cursor_t cursor) {
    const wchar_t* name = IDC_ARROW;
    switch (cursor) {
    case PX_CURSOR_IBEAM:
        name = IDC_IBEAM;
        break;
    case PX_CURSOR_CROSSHAIR:
        name = IDC_CROSS;
        break;
    case PX_CURSOR_POINTING_HAND:
        name = IDC_HAND;
        break;
    case PX_CURSOR_RESIZE_LEFT_RIGHT:
        name = IDC_SIZEWE;
        break;
    case PX_CURSOR_RESIZE_UP_DOWN:
        name = IDC_SIZENS;
        break;
    case PX_CURSOR_ARROW:
        break;
    }
    return LoadCursorW(nullptr, name);
}

// ── coordinates ─────────────────────────────────────────────────────────────────────────────────
// The px API is in device-independent points; Win32 hands us physical pixels once the process is
// per-monitor DPI aware.

vec2 point_from_lparam(const px_window_t* window, LPARAM lparam) {
    const double scale = window->dpi_scale;
    return vec2{GET_X_LPARAM(lparam) / scale, GET_Y_LPARAM(lparam) / scale};
}

RECT physical_rect(const px_window_t* window, rect r) {
    const double scale = window->dpi_scale;
    RECT out;
    out.left = static_cast<LONG>(std::floor(r.x * scale));
    out.top = static_cast<LONG>(std::floor(r.y * scale));
    out.right = static_cast<LONG>(std::ceil(r.right() * scale));
    out.bottom = static_cast<LONG>(std::ceil(r.bottom() * scale));
    return out;
}

// ── render context ──────────────────────────────────────────────────────────────────────────────

// ── IME ─────────────────────────────────────────────────────────────────────────────────────────

px_input_client* input_client_for(px_window_t* window) {
    return (window && window->handler) ? window->handler->get_input_client() : nullptr;
}

std::string composition_string(HIMC himc, DWORD index) {
    const LONG bytes = ImmGetCompositionStringW(himc, index, nullptr, 0);
    if (bytes <= 0) {
        return {};
    }
    std::wstring buffer(static_cast<size_t>(bytes) / sizeof(wchar_t), L'\0');
    ImmGetCompositionStringW(himc, index, buffer.data(), static_cast<DWORD>(bytes));
    return to_utf8(buffer.c_str(), static_cast<int>(buffer.size()));
}

// Keeps the candidate list next to the caret. ST imports ImmSetCandidateWindow for the same job.
void position_candidate_window(px_window_t* window, HIMC himc) {
    px_input_client* client = input_client_for(window);
    if (!client) {
        return;
    }
    px_range_t actual = px_range_t::none();
    const rect caret = client->first_rect_for_range(client->selected_range(), &actual);
    const RECT physical = physical_rect(window, caret);

    CANDIDATEFORM form = {};
    form.dwIndex = 0;
    form.dwStyle = CFS_EXCLUDE;
    form.ptCurrentPos = POINT{physical.left, physical.bottom};
    form.rcArea = physical;
    ImmSetCandidateWindow(himc, &form);
}

// ── paint ───────────────────────────────────────────────────────────────────────────────────────

void paint_window(px_window_t* window) {
    if (!window->handler) {
        return;
    }

    // The update region is read before BeginPaint, which validates it. ST works with real regions
    // rather than a bounding box too -- it imports CreateRectRgn, CombineRgn and GetRegionData.
    std::vector<rect> dirty;
    HRGN region = CreateRectRgn(0, 0, 0, 0);
    if (GetUpdateRgn(window->hwnd, region, FALSE) != ERROR) {
        const DWORD bytes = GetRegionData(region, 0, nullptr);
        if (bytes > 0) {
            std::vector<char> buffer(bytes);
            RGNDATA* data = reinterpret_cast<RGNDATA*>(buffer.data());
            if (GetRegionData(region, bytes, data) == bytes) {
                const RECT* rects = reinterpret_cast<const RECT*>(data->Buffer);
                const double scale = window->dpi_scale;
                for (DWORD i = 0; i < data->rdh.nCount; ++i) {
                    const RECT& r = rects[i];
                    dirty.push_back(rect{r.left / scale, r.top / scale, (r.right - r.left) / scale,
                                         (r.bottom - r.top) / scale});
                }
            }
        }
    }
    DeleteObject(region);

    PAINTSTRUCT ps;
    BeginPaint(window->hwnd, &ps);

    RECT client = {};
    GetClientRect(window->hwnd, &client);
    const double scale = window->dpi_scale;
    const vec2 device{static_cast<double>(client.right - client.left),
                      static_cast<double>(client.bottom - client.top)};
    const rect bounds{0.0, 0.0, device.x / scale, device.y / scale};

    if (dirty.empty()) {
        dirty.push_back(bounds);
    }

    if (window->use_gl && device.x >= 1.0 && device.y >= 1.0) {
        px_win_gl_make_current(window);

        window->handler->pre_paint();
        px_win_dispatch_post_event_callbacks();

        glViewport(0, 0, static_cast<GLsizei>(device.x), static_cast<GLsizei>(device.y));

        const rect paint_bounds = gl_render_context::normalize_dirty_rects(&dirty, bounds);
        if (!window->has_stencil && dirty.size() > 1) {
            // Without stencil the bounding scissor is the only exact clip available. Treat its
            // clean gaps as damaged too, making the fallback conservative rather than corrupting
            // pixels the application still considers preserved.
            dirty.clear();
            dirty.push_back(paint_bounds);
        }
        gl_render_context rc(device, scale, dirty.data(), static_cast<int>(dirty.size()),
                             window->has_stencil);
        window->handler->paint(&rc, rc.paint_bounds(), dirty.data(),
                               static_cast<int>(dirty.size()));
        rc.finish();

        // Single-buffered: glFlush is the whole presentation step. ST imports no SwapBuffers.
        glFlush();

        window->did_first_paint = true;
        window->last_flush = px_now();
    } else {
        // Software path. ST reaches its CPU renderer from here; no CPU rasteriser exists in this
        // experiment, so this only fills with the window background.
        const fcolor& bg = window->background;
        HBRUSH brush =
            CreateSolidBrush(RGB(static_cast<int>(bg.r * 255.0f), static_cast<int>(bg.g * 255.0f),
                                 static_cast<int>(bg.b * 255.0f)));
        FillRect(ps.hdc, &ps.rcPaint, brush);
        DeleteObject(brush);
    }

    EndPaint(window->hwnd, &ps);
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// THE FUNNEL
// ─────────────────────────────────────────────────────────────────────────────────────────────────

void px_win_send_event(px_window_t* window, px_event_t* event) {
    if (!window || !window->handler) {
        return;
    }
    event->window = window;
    window->handler->handle_event(event);

    // send_event's tail: key (0) and character (1) events skip the repaint flush; anything from
    // mouse button (2) upward can trigger one, rate-limited.
    if (window->did_first_paint && event->type >= PX_EVENT_MOUSE_BUTTON) {
        const double now = px_now();
        if (now - window->last_flush > kMinEventFlushInterval) {
            window->handler->pre_paint();
            px_win_flush_dirty_rects(window);
        }
    }

    px_win_dispatch_post_event_callbacks();
}

void px_win_flush_dirty_rects(px_window_t* window) {
    if (!window || window->dirty.empty() || !window->hwnd) {
        return;
    }
    for (const rect& r : window->dirty) {
        const RECT physical = physical_rect(window, r);
        InvalidateRect(window->hwnd, &physical, FALSE);
    }
    window->dirty.clear();
}

void px_win_dispatch_post_event_callbacks() {
    if (post_event_callbacks().empty()) {
        return;
    }
    std::vector<std::function<void()>> pending;
    pending.swap(post_event_callbacks());
    for (const std::function<void()>& fn : pending) {
        fn();
    }
}

px_window_t* px_win_window_from_hwnd(HWND hwnd) {
    for (px_window_t* window : windows_storage()) {
        if (window->hwnd == hwnd) {
            return window;
        }
    }
    return nullptr;
}

const std::vector<px_window_t*>& px_win_all_windows() { return windows_storage(); }

double px_win_dpi_scale_for_window(HWND hwnd) {
    // GetDpiForWindow needs Windows 10 1607. Resolved dynamically so this still links and runs on
    // older systems, where GetDeviceCaps is the fallback. ST declares awareness in its manifest
    // and reads DPI through GetDeviceCaps and EnumDisplayMonitors.
    using PFN_GetDpiForWindow = UINT(WINAPI*)(HWND);
    static PFN_GetDpiForWindow get_dpi = [] {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        return user32 ? reinterpret_cast<PFN_GetDpiForWindow>(
                            GetProcAddress(user32, "GetDpiForWindow"))
                      : nullptr;
    }();

    UINT dpi = 0;
    if (get_dpi && hwnd) {
        dpi = get_dpi(hwnd);
    }
    if (dpi == 0) {
        HDC screen = GetDC(nullptr);
        dpi = screen ? static_cast<UINT>(GetDeviceCaps(screen, LOGPIXELSX)) : 96;
        if (screen) {
            ReleaseDC(nullptr, screen);
        }
    }
    return dpi > 0 ? dpi / 96.0 : 1.0;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// WNDPROC
// ─────────────────────────────────────────────────────────────────────────────────────────────────

namespace {

LRESULT CALLBACK px_wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    // ST looks the window up from a table rather than GWLP_USERDATA: it passes lpParam = 0 to
    // CreateWindowExW and its WndProc opens with a call that maps HWND -> px_window_t*.
    px_window_t* window = px_win_window_from_hwnd(hwnd);
    if (!window) {
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    switch (message) {
    // ── keyboard ────────────────────────────────────────────────────────────────────────────────
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        const bool pressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

        px_event_t e{};
        e.type = PX_EVENT_KEY;
        e.key = px_win_vk_to_px_key(wparam, lparam);
        e.modifiers = px_win_modifiers();
        e.pressed = pressed;
        e.repeat = pressed && (lparam & (LPARAM{1} << 30)) != 0;
        e.window = window;

        const bool consumed = window->handler && window->handler->handle_event(&e);
        px_win_dispatch_post_event_callbacks();

        if (pressed) {
            // TranslateMessage has already queued the matching WM_CHAR by the time this runs, so a
            // consumed binding is suppressed by flagging rather than by skipping translation.
            // Cleared on every key down so a stale flag cannot eat an unrelated character.
            window->suppress_char = consumed;
        }
        if (consumed) {
            return 0;
        }
        break;  // fall through to DefWindowProc so system keys still work
    }

    case WM_CHAR:
    case WM_SYSCHAR: {
        if (window->suppress_char) {
            window->suppress_char = false;
            return 0;
        }
        wchar_t units[2] = {static_cast<wchar_t>(wparam), 0};
        int count = 1;
        // Surrogate pairs arrive as two messages; hold the high half until its partner lands.
        if (units[0] >= 0xD800 && units[0] <= 0xDBFF) {
            window->pending_high_surrogate = units[0];
            return 0;
        }
        if (units[0] >= 0xDC00 && units[0] <= 0xDFFF && window->pending_high_surrogate) {
            units[1] = units[0];
            units[0] = window->pending_high_surrogate;
            window->pending_high_surrogate = 0;
            count = 2;
        }

        const std::string utf8 = to_utf8(units, count);
        if (!utf8.empty()) {
            px_event_t e{};
            e.type = PX_EVENT_CHARACTER;
            e.modifiers = px_win_modifiers();
            px_set_event_text(&e, utf8.data(), utf8.size());
            px_win_send_event(window, &e);
        }
        return 0;
    }

    // ── IME ─────────────────────────────────────────────────────────────────────────────────────
    case WM_IME_SETCONTEXT:
        // Suppress the system composition window; the editor draws its own preedit.
        return DefWindowProcW(hwnd, message, wparam, lparam & ~ISC_SHOWUICOMPOSITIONWINDOW);

    case WM_IME_STARTCOMPOSITION:
        window->composing = true;
        return 0;

    case WM_IME_COMPOSITION: {
        HIMC himc = ImmGetContext(hwnd);
        if (!himc) {
            return 0;
        }
        px_input_client* client = input_client_for(window);
        if (lparam & GCS_RESULTSTR) {
            const std::string result = composition_string(himc, GCS_RESULTSTR);
            if (!result.empty()) {
                if (client) {
                    client->insert_text(result.c_str(), px_range_t::none());
                } else {
                    px_event_t e{};
                    e.type = PX_EVENT_CHARACTER;
                    px_set_event_text(&e, result.data(), result.size());
                    px_win_send_event(window, &e);
                }
            }
        }
        if (lparam & GCS_COMPSTR) {
            const std::string preedit = composition_string(himc, GCS_COMPSTR);
            if (client) {
                client->set_marked_text(preedit.c_str(), px_range_t::none(), px_range_t::none());
            }
        }
        position_candidate_window(window, himc);
        ImmReleaseContext(hwnd, himc);
        return 0;
    }

    case WM_IME_ENDCOMPOSITION:
        window->composing = false;
        if (px_input_client* client = input_client_for(window)) {
            client->unmark_text();
        }
        return 0;

    // ── mouse ───────────────────────────────────────────────────────────────────────────────────
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP: {
        px_mouse_button button = PX_MOUSE_LEFT;
        bool pressed = false;
        int clicks = 1;
        switch (message) {
        case WM_LBUTTONDOWN:
            pressed = true;
            break;
        case WM_LBUTTONDBLCLK:
            pressed = true;
            clicks = 2;
            break;
        case WM_LBUTTONUP:
            break;
        case WM_RBUTTONDOWN:
            button = PX_MOUSE_RIGHT;
            pressed = true;
            break;
        case WM_RBUTTONDBLCLK:
            button = PX_MOUSE_RIGHT;
            pressed = true;
            clicks = 2;
            break;
        case WM_RBUTTONUP:
            button = PX_MOUSE_RIGHT;
            break;
        case WM_MBUTTONDOWN:
            button = PX_MOUSE_MIDDLE;
            pressed = true;
            break;
        case WM_MBUTTONDBLCLK:
            button = PX_MOUSE_MIDDLE;
            pressed = true;
            clicks = 2;
            break;
        case WM_MBUTTONUP:
            button = PX_MOUSE_MIDDLE;
            break;
        case WM_XBUTTONDOWN:
            button = GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? PX_MOUSE_X1 : PX_MOUSE_X2;
            pressed = true;
            break;
        case WM_XBUTTONUP:
            button = GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? PX_MOUSE_X1 : PX_MOUSE_X2;
            break;
        default:
            break;
        }

        if (pressed) {
            SetFocus(hwnd);
            SetCapture(hwnd);
        } else if (GetCapture() == hwnd) {
            ReleaseCapture();
        }

        px_event_t e{};
        e.type = PX_EVENT_MOUSE_BUTTON;
        e.pos = point_from_lparam(window, lparam);
        e.button = button;
        e.pressed = pressed;
        e.click_count = clicks;
        e.modifiers = px_win_modifiers();
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!window->tracking_mouse) {
            TRACKMOUSEEVENT track = {};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            if (TrackMouseEvent(&track)) {
                window->tracking_mouse = true;
            }
        }

        px_event_t e{};
        e.type = PX_EVENT_MOUSE_MOTION;
        e.pos = point_from_lparam(window, lparam);
        e.modifiers = px_win_modifiers();
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_MOUSELEAVE: {
        window->tracking_mouse = false;
        px_event_t e{};
        e.type = PX_EVENT_MOUSE_LEAVE;
        e.modifiers = px_win_modifiers();
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL: {
        // Wheel coordinates arrive in screen space, unlike every other mouse message.
        POINT screen{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
        ScreenToClient(hwnd, &screen);

        const double delta =
            static_cast<double>(GET_WHEEL_DELTA_WPARAM(wparam)) / WHEEL_DELTA * 16.0 * 3.0;

        px_event_t e{};
        e.type = PX_EVENT_SCROLL;
        e.pos = vec2{screen.x / window->dpi_scale, screen.y / window->dpi_scale};
        e.modifiers = px_win_modifiers();
        e.precise_scroll = false;
        // A positive WM_MOUSEWHEEL delta means "away from the user", which matches the macOS
        // convention of positive dy scrolling content up.
        e.scroll_delta = message == WM_MOUSEWHEEL ? vec2{0.0, delta} : vec2{-delta, 0.0};
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_CAPTURECHANGED: {
        px_event_t e{};
        e.type = PX_EVENT_CAPTURE_LOST;
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_SETCURSOR:
        if (LOWORD(lparam) == HTCLIENT) {
            POINT cursor;
            GetCursorPos(&cursor);
            ScreenToClient(hwnd, &cursor);
            const px_cursor_t wanted =
                window->handler ? window->handler->calculate_cursor(vec2{
                                      cursor.x / window->dpi_scale, cursor.y / window->dpi_scale})
                                : PX_CURSOR_ARROW;
            SetCursor(win_cursor(wanted));
            return TRUE;
        }
        break;

    // ── window ──────────────────────────────────────────────────────────────────────────────────
    case WM_PAINT:
        paint_window(window);
        return 0;

    case WM_ERASEBKGND:
        // Everything is painted in WM_PAINT; erasing first would only flicker.
        return 1;

    case WM_TIMER:
        if (wparam == kAnimationTimerId) {
            if (window->handler && !window->closing) {
                window->handler->animation_tick(px_now());
            }
            px_win_flush_dirty_rects(window);
            return 0;
        }
        break;

    case WM_SIZE: {
        RECT client = {};
        GetClientRect(hwnd, &client);
        const double scale = window->dpi_scale;
        const vec2 size{(client.right - client.left) / scale,
                        (client.bottom - client.top) / scale};

        px_mark_rect_dirty(window, rect{0.0, 0.0, size.x, size.y});

        px_event_t e{};
        e.type = PX_EVENT_RESIZE;
        e.size = size;
        e.dpi_scale_factor = scale;
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_DPICHANGED: {
        window->dpi_scale = LOWORD(wparam) / 96.0;
        // lParam carries the frame Windows wants the window moved to so its physical size is
        // preserved across the DPI change.
        const RECT* suggested = reinterpret_cast<const RECT*>(lparam);
        if (suggested) {
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left, suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }

        px_event_t e{};
        e.type = PX_EVENT_DPI_CHANGED;
        e.size = px_window_size(window);
        e.dpi_scale_factor = window->dpi_scale;
        px_win_send_event(window, &e);
        px_mark_dirty(window);
        return 0;
    }

    case WM_SETFOCUS: {
        px_event_t e{};
        e.type = PX_EVENT_FOCUS_GAINED;
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_KILLFOCUS: {
        px_event_t e{};
        e.type = PX_EVENT_FOCUS_LOST;
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED: {
        px_event_t e{};
        e.type = PX_EVENT_SETTINGS_CHANGED;
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_DROPFILES: {
        HDROP drop = reinterpret_cast<HDROP>(wparam);
        const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        drop_paths().storage.clear();
        for (UINT i = 0; i < count; ++i) {
            const UINT length = DragQueryFileW(drop, i, nullptr, 0);
            std::wstring path(length, L'\0');
            DragQueryFileW(drop, i, path.data(), length + 1);
            drop_paths().storage.push_back(to_utf8(path.c_str(), static_cast<int>(path.size())));
        }
        drop_paths().rebuild_pointers();

        POINT where = {};
        DragQueryPoint(drop, &where);
        DragFinish(drop);

        const vec2 pos{where.x / window->dpi_scale, where.y / window->dpi_scale};
        if (window->handler) {
            window->handler->drag_drop_accept(pos, drop_paths().data(), drop_paths().count());
        }

        px_event_t e{};
        e.type = PX_EVENT_DROP_FILES;
        e.pos = pos;
        e.paths = drop_paths().data();
        e.path_count = drop_paths().count();
        px_win_send_event(window, &e);
        return 0;
    }

    case WM_CLOSE:
        if (window->handler && !window->handler->can_close_without_prompt()) {
            px_window_t* target = window;
            window->handler->try_close([target](bool should_close) {
                if (should_close) {
                    DestroyWindow(target->hwnd);
                }
            });
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        KillTimer(hwnd, kAnimationTimerId);

        px_event_t e{};
        e.type = PX_EVENT_DESTROY;
        px_win_send_event(window, &e);
        window->closing = true;

        // Equivalent of -[PXApplicationDelegate applicationShouldTerminateAfterLastWindowClosed:].
        const bool any_left = std::any_of(windows_storage().begin(), windows_storage().end(),
                                          [](const px_window_t* w) { return !w->closing; });
        if (!any_left) {
            PostQuitMessage(0);
        }
        return 0;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

}  // namespace

void px_win_register_class() {
    if (g_class_registered) {
        return;
    }
    g_class_registered = true;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    // CS_DBLCLKS and nothing else, as in ST. In particular no CS_HREDRAW/CS_VREDRAW: those force a
    // full invalidation on every resize, which would defeat the dirty-rect model.
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = &px_wnd_proc;
    wc.hInstance = nullptr;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kPxWindowClass;
    RegisterClassExW(&wc);
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FLAT API: WINDOWS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

px_window_t* px_create_window(px_window_event_handler* handler,
                              px_window_t* parent,
                              double width,
                              double height,
                              const char* title,
                              fcolor background,
                              uint32_t flags) {
    px_win_register_class();

    px_window_t* window = new px_window_t();
    window->handler = handler ? handler : &dummy_handler();
    window->background = background;

    wchar_t no_gl[8] = {};
    window->use_gl = GetEnvironmentVariableW(L"PX_NO_GL", no_gl, 8) == 0;

    DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    if (flags & PX_WINDOW_TITLED) style |= WS_CAPTION | WS_SYSMENU;
    if (flags & PX_WINDOW_RESIZABLE) style |= WS_THICKFRAME;
    if (flags & PX_WINDOW_MINIATURIZABLE) style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    if (style == (WS_CLIPCHILDREN | WS_CLIPSIBLINGS)) style |= WS_POPUP;

    // The entry goes in before CreateWindowExW, but the HWND link is only established once that
    // returns, so the messages Windows sends during creation (WM_NCCREATE, WM_CREATE, the first
    // WM_SIZE) find no match and fall through to DefWindowProcW. That is harmless -- the window is
    // fully invalidated when it is first shown -- and it is the same characteristic ST has, since
    // it passes lpParam = 0 and looks the window up from a table rather than from the
    // CREATESTRUCT.
    windows_storage().push_back(window);

    const double scale = px_win_dpi_scale_for_window(nullptr);
    window->dpi_scale = scale;

    RECT frame = {0, 0, static_cast<LONG>(width * scale), static_cast<LONG>(height * scale)};
    AdjustWindowRectEx(&frame, style, FALSE, 0);

    const std::wstring wide_title = to_utf16(title);
    window->hwnd =
        CreateWindowExW(0, kPxWindowClass, wide_title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT,
                        frame.right - frame.left, frame.bottom - frame.top,
                        parent ? parent->hwnd : nullptr, nullptr, nullptr, nullptr);
    if (!window->hwnd) {
        std::println(stderr, "px: CreateWindowExW failed (error {})", GetLastError());
        std::vector<px_window_t*>& all = windows_storage();
        all.erase(std::remove(all.begin(), all.end(), window), all.end());
        delete window;
        return nullptr;
    }

    // Now that there is an HWND, ask for its real monitor DPI.
    window->dpi_scale = px_win_dpi_scale_for_window(window->hwnd);

    if (window->use_gl && !px_win_gl_create(window)) {
        window->use_gl = false;
    }

    DragAcceptFiles(window->hwnd, TRUE);
    SetTimer(window->hwnd, kAnimationTimerId, kAnimationIntervalMs, nullptr);
    return window;
}

void px_destroy_window(px_window_t* window) {
    if (!window) {
        return;
    }
    px_win_gl_destroy(window);
    if (window->hwnd && !window->closing) {
        DestroyWindow(window->hwnd);
    }
    window->hwnd = nullptr;

    std::vector<px_window_t*>& all = windows_storage();
    all.erase(std::remove(all.begin(), all.end(), window), all.end());
    delete window;
}

void px_show_window(px_window_t* window) {
    if (window && window->hwnd) {
        ShowWindow(window->hwnd, SW_SHOW);
        SetForegroundWindow(window->hwnd);
        SetFocus(window->hwnd);
    }
}

void px_show_window_without_focus(px_window_t* window) {
    if (window && window->hwnd) {
        ShowWindow(window->hwnd, SW_SHOWNOACTIVATE);
    }
}

void px_hide_window(px_window_t* window) {
    if (window && window->hwnd) {
        ShowWindow(window->hwnd, SW_HIDE);
    }
}

void px_close_window(px_window_t* window) {
    if (window && window->hwnd) {
        PostMessageW(window->hwnd, WM_CLOSE, 0, 0);
    }
}

void px_set_window_title(px_window_t* window, const char* title) {
    if (window && window->hwnd) {
        SetWindowTextW(window->hwnd, to_utf16(title).c_str());
    }
}

vec2 px_window_size(px_window_t* window) {
    if (!window || !window->hwnd) {
        return vec2{};
    }
    RECT client = {};
    GetClientRect(window->hwnd, &client);
    const double scale = window->dpi_scale;
    return vec2{(client.right - client.left) / scale, (client.bottom - client.top) / scale};
}

vec2 px_window_position(px_window_t* window) {
    if (!window || !window->hwnd) {
        return vec2{};
    }
    RECT frame = {};
    GetWindowRect(window->hwnd, &frame);
    const double scale = window->dpi_scale;
    return vec2{frame.left / scale, frame.top / scale};
}

void px_set_window_size(px_window_t* window, double width, double height) {
    if (!window || !window->hwnd) {
        return;
    }
    const double scale = window->dpi_scale;
    RECT frame = {0, 0, static_cast<LONG>(width * scale), static_cast<LONG>(height * scale)};
    AdjustWindowRectEx(&frame, static_cast<DWORD>(GetWindowLongPtrW(window->hwnd, GWL_STYLE)),
                       FALSE, static_cast<DWORD>(GetWindowLongPtrW(window->hwnd, GWL_EXSTYLE)));
    SetWindowPos(window->hwnd, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void px_set_window_position(px_window_t* window, vec2 position) {
    if (!window || !window->hwnd) {
        return;
    }
    const double scale = window->dpi_scale;
    SetWindowPos(window->hwnd, nullptr, static_cast<int>(position.x * scale),
                 static_cast<int>(position.y * scale), 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

double px_window_dpi_scale_factor(px_window_t* window) { return window ? window->dpi_scale : 1.0; }

void px_set_full_screen(px_window_t* window, bool full_screen) {
    if (!window || !window->hwnd) {
        return;
    }
    // Borderless-on-monitor, the usual Win32 stand-in for a real full-screen mode. The saved frame
    // and style live on the window, not in statics, so two windows can toggle independently.
    if (full_screen) {
        if (window->saved_style != 0) {
            return;  // already full screen
        }
        window->saved_style = GetWindowLongPtrW(window->hwnd, GWL_STYLE);
        GetWindowRect(window->hwnd, &window->saved_frame);

        MONITORINFO info = {};
        info.cbSize = sizeof(info);
        GetMonitorInfoW(MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTONEAREST), &info);

        SetWindowLongPtrW(window->hwnd, GWL_STYLE,
                          window->saved_style & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowPos(window->hwnd, HWND_TOP, info.rcMonitor.left, info.rcMonitor.top,
                     info.rcMonitor.right - info.rcMonitor.left,
                     info.rcMonitor.bottom - info.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else if (window->saved_style != 0) {
        const RECT& frame = window->saved_frame;
        SetWindowLongPtrW(window->hwnd, GWL_STYLE, window->saved_style);
        SetWindowPos(window->hwnd, nullptr, frame.left, frame.top, frame.right - frame.left,
                     frame.bottom - frame.top,
                     SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        window->saved_style = 0;
    }
}

void px_mark_rect_dirty(px_window_t* window, rect r) {
    if (window && !r.empty()) {
        window->dirty.push_back(r);
    }
}

void px_mark_dirty(px_window_t* window) {
    if (!window) {
        return;
    }
    const vec2 size = px_window_size(window);
    px_mark_rect_dirty(window, rect{0.0, 0.0, size.x, size.y});
}

void px_set_cursor(px_window_t* window, px_cursor_t cursor) {
    if (window) {
        window->cursor = cursor;
        SetCursor(win_cursor(cursor));
    }
}

void px_reset_cursor(px_window_t* window) {
    if (window && window->hwnd) {
        // Provokes a fresh WM_SETCURSOR, which re-asks the handler. Nudging the pointer with
        // SetCursorPos would also work but moves the user's cursor, which is not ours to move.
        SendMessageW(window->hwnd, WM_SETCURSOR, reinterpret_cast<WPARAM>(window->hwnd),
                     MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
    }
}

void px_callback_after_event(std::function<void()> fn) {
    post_event_callbacks().push_back(std::move(fn));
}
