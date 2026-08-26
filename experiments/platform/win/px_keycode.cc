// Native keycode -> portable px_key.
//
// This file is nearly an identity function, and that is the point. px_key's named-key space is
// Win32 virtual-key numbering, so the Windows backend mostly just sets the PX_KEY_NAMED bit while
// the Cocoa backend does real translation work. ST made the same call: keycode_to_px_key on macOS
// maps Carbon keycodes onto VK values, and nothing on Windows has to.

#include "experiments/platform/win/px_win_private.h"

namespace {

bool is_named_vk(WPARAM vk) {
  switch (vk) {
    case VK_BACK:
    case VK_TAB:
    case VK_RETURN:
    case VK_PAUSE:
    case VK_ESCAPE:
    case VK_SPACE:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_INSERT:
    case VK_DELETE:
    case VK_F1:
    case VK_F2:
    case VK_F3:
    case VK_F4:
    case VK_F5:
    case VK_F6:
    case VK_F7:
    case VK_F8:
    case VK_F9:
    case VK_F10:
    case VK_F11:
    case VK_F12:
      return true;
    default:
      return false;
  }
}

}  // namespace

uint32_t px_win_modifiers() {
  uint32_t mods = 0;
  if (GetKeyState(VK_SHIFT) & 0x8000) mods |= PX_MOD_SHIFT;
  if (GetKeyState(VK_CONTROL) & 0x8000) mods |= PX_MOD_CONTROL;
  if (GetKeyState(VK_MENU) & 0x8000) mods |= PX_MOD_ALT;
  if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) mods |= PX_MOD_SUPER;
  if (GetKeyState(VK_CAPITAL) & 0x0001) mods |= PX_MOD_CAPS_LOCK;
  return mods;
}

px_key px_win_vk_to_px_key(WPARAM vk, LPARAM lparam) {
  // Bit 24 of lParam is the extended-key flag. ST uses exactly this test to split the numpad's
  // Enter off from the main one: it compares the looked-up key against 0x8000000D and substitutes
  // 0x8000010F when the bit is set.
  constexpr LPARAM kExtendedKey = LPARAM{1} << 24;
  if (vk == VK_RETURN && (lparam & kExtendedKey)) {
    return PX_KEY_KEYPAD_ENTER;
  }

  if (is_named_vk(vk)) {
    return PX_KEY_NAMED | static_cast<px_key>(vk);
  }

  // Printable keys report their unshifted character, so a binding table can hold 'p' and match
  // regardless of Shift -- Shift lives in the modifier mask.
  const UINT ch = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_CHAR);
  if (ch == 0) {
    return PX_KEY_NONE;
  }
  // MAPVK_VK_TO_CHAR sets the high bit for dead keys; there is no single codepoint to report.
  if (ch & 0x80000000u) {
    return PX_KEY_NONE;
  }
  if (ch >= 'A' && ch <= 'Z') {
    return static_cast<px_key>(ch - 'A' + 'a');
  }
  if (ch < 0x20) {
    return PX_KEY_NONE;
  }
  return static_cast<px_key>(ch);
}
