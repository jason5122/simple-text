// Native keycode -> portable px_key.
//
// px_key's named-key space is Win32 virtual-key numbering, so like the Cocoa backend this one
// translates onto VK values rather than being them natively. The one thing X11/GDK gets for free
// that Win32 does not: keypad Enter is its own keysym (GDK_KEY_KP_Enter), so there is no lParam
// extended-key bit to test the way px_win_vk_to_px_key does.
//
// The confirmed API choice for the printable case is gdk_keymap_translate_keyboard_state, called
// with an empty modifier mask against the raw hardware_keycode -- the X11 analogue of Win32's
// MapVirtualKeyW(vk, MAPVK_VK_TO_CHAR). GDK's own event->keyval is already shift-aware (Shift+a
// yields keyval 'A'), which is the wrong currency for a binding table that wants the unshifted
// character with Shift folded into the modifier mask instead.

#include "experiments/platform/linux/px_linux_private.h"

namespace {

px_key named_key_for_keyval(guint keyval) {
  switch (keyval) {
    case GDK_KEY_BackSpace: return PX_KEY_BACKSPACE;
    case GDK_KEY_Tab: return PX_KEY_TAB;
    case GDK_KEY_Return: return PX_KEY_ENTER;
    case GDK_KEY_KP_Enter: return PX_KEY_KEYPAD_ENTER;
    case GDK_KEY_Pause: return PX_KEY_PAUSE;
    case GDK_KEY_Escape: return PX_KEY_ESCAPE;
    case GDK_KEY_space: return PX_KEY_SPACE;
    case GDK_KEY_Page_Up: return PX_KEY_PAGE_UP;
    case GDK_KEY_Page_Down: return PX_KEY_PAGE_DOWN;
    case GDK_KEY_End: return PX_KEY_END;
    case GDK_KEY_Home: return PX_KEY_HOME;
    case GDK_KEY_Left: return PX_KEY_LEFT;
    case GDK_KEY_Up: return PX_KEY_UP;
    case GDK_KEY_Right: return PX_KEY_RIGHT;
    case GDK_KEY_Down: return PX_KEY_DOWN;
    case GDK_KEY_Insert: return PX_KEY_INSERT;
    case GDK_KEY_Delete: return PX_KEY_DELETE;
    case GDK_KEY_F1: return PX_KEY_F1;
    case GDK_KEY_F2: return PX_KEY_F2;
    case GDK_KEY_F3: return PX_KEY_F3;
    case GDK_KEY_F4: return PX_KEY_F4;
    case GDK_KEY_F5: return PX_KEY_F5;
    case GDK_KEY_F6: return PX_KEY_F6;
    case GDK_KEY_F7: return PX_KEY_F7;
    case GDK_KEY_F8: return PX_KEY_F8;
    case GDK_KEY_F9: return PX_KEY_F9;
    case GDK_KEY_F10: return PX_KEY_F10;
    case GDK_KEY_F11: return PX_KEY_F11;
    case GDK_KEY_F12: return PX_KEY_F12;
    default: return PX_KEY_NONE;
  }
}

}  // namespace

uint32_t px_linux_modifiers(GdkModifierType state) {
  uint32_t mods = 0;
  if (state & GDK_SHIFT_MASK) mods |= PX_MOD_SHIFT;
  if (state & GDK_CONTROL_MASK) mods |= PX_MOD_CONTROL;
  if (state & GDK_MOD1_MASK) mods |= PX_MOD_ALT;
  if (state & (GDK_SUPER_MASK | GDK_META_MASK)) mods |= PX_MOD_SUPER;
  if (state & GDK_LOCK_MASK) mods |= PX_MOD_CAPS_LOCK;
  return mods;
}

px_key px_linux_keyval_to_px_key(GdkEventKey* event) {
  // Named keys first, straight off the (shift-invariant) keyval GDK already computed.
  if (px_key named = named_key_for_keyval(event->keyval); named != PX_KEY_NONE) {
    return named;
  }

  // Re-translate the physical key with an empty modifier state to recover the unshifted glyph.
  // event->keyval alone cannot be un-shifted after the fact -- GDK has already applied Shift and
  // the active group by the time the event exists.
  GdkKeymap* keymap = gdk_keymap_get_for_display(gdk_window_get_display(event->window));
  guint unshifted = 0;
  if (!gdk_keymap_translate_keyboard_state(keymap, event->hardware_keycode,
                                           static_cast<GdkModifierType>(0), event->group,
                                           &unshifted, nullptr, nullptr, nullptr)) {
    return PX_KEY_NONE;
  }

  const gunichar ch = gdk_keyval_to_unicode(unshifted);
  if (ch == 0 || ch < 0x20) {
    return PX_KEY_NONE;
  }
  if (ch >= 'A' && ch <= 'Z') {
    return static_cast<px_key>(ch - 'A' + 'a');
  }
  return static_cast<px_key>(ch);
}
