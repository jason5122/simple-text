// Process-level half of the Win32 backend: init, the message loop, timers, and the odds and ends
// of the flat API that are not per-window.
//
// The loop is PeekMessageW + MsgWaitForMultipleObjectsEx rather than GetMessageW, and that is not
// a stylistic choice -- ST's import table has PeekMessageW, DispatchMessageW, TranslateMessage and
// MsgWaitForMultipleObjectsEx, and no GetMessageW at all. The reason is visible in the handler
// interface: pre_sleep() needs a hook at the moment the queue drains and the thread is about to
// block, and GetMessageW gives you nowhere to put it.

#include "experiments/platform/win/px_win_private.h"

#include <shellapi.h>

#include <chrono>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace {

px_application_event_handler* g_app_handler = nullptr;
px_application_event_handler g_default_app_handler;
std::chrono::steady_clock::time_point g_start;
bool g_quit = false;

px_application_event_handler& app_handler() {
  return g_app_handler ? *g_app_handler : g_default_app_handler;
}

// SetTimer's callbacks are keyed by an id; this maps them back to the closures.
std::map<UINT_PTR, std::function<void()>>& timers() {
  static std::map<UINT_PTR, std::function<void()>> map;
  return map;
}

UINT_PTR g_next_timer_id = 1;

void CALLBACK timer_proc(HWND hwnd, UINT message, UINT_PTR id, DWORD time) {
  (void)hwnd;
  (void)message;
  (void)time;
  auto it = timers().find(id);
  if (it == timers().end()) {
    return;
  }
  // Move the closure out and cancel the timer before running it, so a callback that schedules
  // another timeout cannot disturb the map mid-iteration.
  std::function<void()> fn = std::move(it->second);
  timers().erase(it);
  KillTimer(nullptr, id);
  fn();
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

// Per-monitor DPI awareness without a manifest. ST declares it in its manifest instead; doing it
// programmatically keeps this experiment to a single binary with no side files.
void enable_dpi_awareness() {
  using DpiAwarenessContext = HANDLE;
  using PFN_SetProcessDpiAwarenessContext = BOOL(WINAPI*)(DpiAwarenessContext);

  if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
    auto set_context = reinterpret_cast<PFN_SetProcessDpiAwarenessContext>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (set_context) {
      // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
      if (set_context(reinterpret_cast<DpiAwarenessContext>(-4))) {
        return;
      }
    }
  }
  SetProcessDPIAware();
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FLAT API: PROCESS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

void px_init(const char* app_name, const char* bundle_id, int argc, char** argv, uint32_t flags) {
  (void)app_name;
  (void)bundle_id;
  (void)argc;
  (void)argv;
  (void)flags;

  g_start = std::chrono::steady_clock::now();
  enable_dpi_awareness();
  px_win_register_class();
}

void px_set_application_event_handler(px_application_event_handler* handler) {
  g_app_handler = handler;
}

void px_run_event_loop() {
  g_quit = false;

  while (!g_quit) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        g_quit = true;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    if (g_quit) {
      break;
    }

    // The queue is empty. Give every window a chance to settle before the thread blocks: this is
    // what pre_sleep() exists for, and why the loop cannot be GetMessageW.
    // Iterated over a copy, because a handler is allowed to destroy a window from here.
    const std::vector<px_window_t*> snapshot = px_win_all_windows();
    for (px_window_t* window : snapshot) {
      if (window->handler && !window->closing) {
        window->handler->pre_sleep();
      }
      px_win_flush_dirty_rects(window);
    }

    MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT,
                                MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
  }
}

void px_exit_event_loop() {
  g_quit = true;
  PostQuitMessage(0);
}

void px_set_timeout(std::function<void()> fn, int milliseconds) {
  const UINT_PTR id = SetTimer(nullptr, g_next_timer_id++, static_cast<UINT>(milliseconds),
                               &timer_proc);
  if (id != 0) {
    timers()[id] = std::move(fn);
  }
}

bool px_os_in_dark_mode() {
  // The documented signal is AppsUseLightTheme under Personalize; absent means light.
  DWORD light = 1;
  DWORD size = sizeof(light);
  const LSTATUS status = RegGetValueW(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme",
      RRF_RT_REG_DWORD, nullptr, &light, &size);
  return status == ERROR_SUCCESS && light == 0;
}

double px_caret_blink_time() {
  const UINT blink_ms = GetCaretBlinkTime();
  if (blink_ms == 0 || blink_ms == INFINITE) {
    return 0.0;  // "do not blink"
  }
  return blink_ms / 1000.0;
}

void px_show_error(px_window_t* parent, const char* message) {
  MessageBoxW(parent ? parent->hwnd : nullptr, to_utf16(message).c_str(), L"Error",
              MB_OK | MB_ICONERROR);
}

void px_open_url(const char* url) {
  if (url) {
    ShellExecuteW(nullptr, L"open", to_utf16(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
}

double px_now() {
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - g_start;
  return elapsed.count();
}
