// Shared internals of the Win32 backend. Portable code sees px_window_t as an opaque forward
// declaration, exactly as it does on macOS.

#pragma once

#include <windows.h>

#include <vector>

#include "experiments/platform/px.h"

// ST registers a class literally named PX_WINDOW_CLASS. Its WNDCLASSEXW, read out of the binary at
// 0x1401c5f3d, is:
//     cbSize        = 0x50
//     style         = 0x08  (CS_DBLCLKS, and nothing else -- note the absence of CS_HREDRAW /
//                            CS_VREDRAW, which would force a full repaint on every resize and
//                            defeat the dirty-rect model)
//     lpfnWndProc   = 0x1401be53a
//     cbClsExtra    = 0
//     cbWndExtra    = 0
//     hInstance     = NULL
//     hIcon/hIconSm = ExtractIconExW(...)
//     hCursor       = LoadCursorW(NULL, IDC_ARROW)
//     hbrBackground = CreateSolidBrush(<window background>)
//     lpszMenuName  = NULL
//     lpszClassName = L"PX_WINDOW_CLASS"
inline constexpr wchar_t kPxWindowClass[] = L"PX_WINDOW_CLASS";

struct px_window_t {
  HWND hwnd = nullptr;
  HDC hdc = nullptr;
  HGLRC hglrc = nullptr;

  px_window_event_handler* handler = nullptr;

  bool did_first_paint = false;
  bool closing = false;
  bool tracking_mouse = false;
  bool use_gl = true;

  // Device pixels per point. WM_DPICHANGED keeps it current.
  double dpi_scale = 1.0;

  fcolor background{0.0f, 0.0f, 0.0f, 1.0f};
  px_cursor_t cursor = PX_CURSOR_ARROW;
  HBRUSH background_brush = nullptr;

  // Pending regions in window-space points, drained into InvalidateRect.
  std::vector<rect> dirty;

  double last_flush = 0.0;

  // Set between WM_IME_STARTCOMPOSITION and WM_IME_ENDCOMPOSITION.
  bool composing = false;

  // Set when a WM_KEYDOWN was consumed by a binding, so the WM_CHAR that TranslateMessage already
  // queued for it does not also get typed. Cleared on the next key down either way.
  bool suppress_char = false;

  // WM_CHAR delivers astral codepoints as two messages.
  wchar_t pending_high_surrogate = 0;

  // Saved across a borderless-fullscreen toggle.
  RECT saved_frame = {};
  LONG_PTR saved_style = 0;
};

// px_window.cc
void px_win_send_event(px_window_t* window, px_event_t* event);
void px_win_flush_dirty_rects(px_window_t* window);
void px_win_dispatch_post_event_callbacks();
px_window_t* px_win_window_from_hwnd(HWND hwnd);
const std::vector<px_window_t*>& px_win_all_windows();
void px_win_register_class();
double px_win_dpi_scale_for_window(HWND hwnd);

// px_gl.cc
bool px_win_gl_create(px_window_t* window);
void px_win_gl_destroy(px_window_t* window);
void px_win_gl_make_current(px_window_t* window);

// px_keycode.cc
uint32_t px_win_modifiers();
px_key px_win_vk_to_px_key(WPARAM vk, LPARAM lparam);
