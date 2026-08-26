// Native keycode -> portable px_key.
//
// ST does this translation inside the platform layer, in keycode_to_px_key(NSString*, unsigned
// short, unsigned, unsigned*), and normalises onto Win32 virtual-key numbering: the Mac backend
// translates *to* VK values, so a key binding parsed once means the same thing on both platforms.
// Character keys stay as their unshifted codepoint, which is why a Sublime keymap can spell a
// chord as "ctrl+shift+p" and a named key as "pagedown".

#include "experiments/platform/mac/px_mac_private.h"

namespace {

// Carbon kVK_* values, spelled out so this file does not have to pull in Carbon.
enum : unsigned short {
  kVKReturn = 0x24,
  kVKTab = 0x30,
  kVKSpace = 0x31,
  kVKBackspace = 0x33,   // "Delete" on an Apple keyboard
  kVKEscape = 0x35,
  kVKKeypadClear = 0x47,
  kVKKeypadEnter = 0x4C,
  kVKF5 = 0x60,
  kVKF6 = 0x61,
  kVKF7 = 0x62,
  kVKF3 = 0x63,
  kVKF8 = 0x64,
  kVKF9 = 0x65,
  kVKF11 = 0x67,
  kVKF10 = 0x6D,
  kVKF12 = 0x6F,
  kVKHelp = 0x72,  // Insert
  kVKHome = 0x73,
  kVKPageUp = 0x74,
  kVKForwardDelete = 0x75,
  kVKF4 = 0x76,
  kVKEnd = 0x77,
  kVKF2 = 0x78,
  kVKPageDown = 0x79,
  kVKF1 = 0x7A,
  kVKLeftArrow = 0x7B,
  kVKRightArrow = 0x7C,
  kVKDownArrow = 0x7D,
  kVKUpArrow = 0x7E,
};

px_key named_key_for_keycode(unsigned short code) {
  switch (code) {
    case kVKReturn: return PX_KEY_ENTER;
    case kVKKeypadEnter: return PX_KEY_KEYPAD_ENTER;
    case kVKTab: return PX_KEY_TAB;
    case kVKSpace: return PX_KEY_SPACE;
    case kVKBackspace: return PX_KEY_BACKSPACE;
    case kVKForwardDelete: return PX_KEY_DELETE;
    case kVKEscape: return PX_KEY_ESCAPE;
    case kVKHelp: return PX_KEY_INSERT;
    case kVKHome: return PX_KEY_HOME;
    case kVKEnd: return PX_KEY_END;
    case kVKPageUp: return PX_KEY_PAGE_UP;
    case kVKPageDown: return PX_KEY_PAGE_DOWN;
    case kVKLeftArrow: return PX_KEY_LEFT;
    case kVKRightArrow: return PX_KEY_RIGHT;
    case kVKUpArrow: return PX_KEY_UP;
    case kVKDownArrow: return PX_KEY_DOWN;
    case kVKKeypadClear: return PX_KEY_PAUSE;
    case kVKF1: return PX_KEY_F1;
    case kVKF2: return PX_KEY_F2;
    case kVKF3: return PX_KEY_F3;
    case kVKF4: return PX_KEY_F4;
    case kVKF5: return PX_KEY_F5;
    case kVKF6: return PX_KEY_F6;
    case kVKF7: return PX_KEY_F7;
    case kVKF8: return PX_KEY_F8;
    case kVKF9: return PX_KEY_F9;
    case kVKF10: return PX_KEY_F10;
    case kVKF11: return PX_KEY_F11;
    case kVKF12: return PX_KEY_F12;
    default: return PX_KEY_NONE;
  }
}

// AppKit reports the arrow keys and friends as private-use codepoints in
// charactersIgnoringModifiers. Anything in that block is a function key we either mapped above or
// do not care about, and must never be reported as a character.
bool is_private_use(unichar c) {
  return c >= 0xE000 && c <= 0xF8FF;
}

}  // namespace

uint32_t px_mac_modifiers_from_ns(NSEventModifierFlags flags) {
  uint32_t mods = 0;
  if (flags & NSEventModifierFlagShift) mods |= PX_MOD_SHIFT;
  if (flags & NSEventModifierFlagControl) mods |= PX_MOD_CONTROL;
  if (flags & NSEventModifierFlagOption) mods |= PX_MOD_ALT;
  if (flags & NSEventModifierFlagCommand) mods |= PX_MOD_SUPER;
  if (flags & NSEventModifierFlagCapsLock) mods |= PX_MOD_CAPS_LOCK;
  return mods;
}

px_key px_mac_keycode_to_px_key(NSString* chars_ignoring_modifiers,
                                unsigned short key_code,
                                NSEventModifierFlags flags) {
  (void)flags;

  if (px_key named = named_key_for_keycode(key_code); named != PX_KEY_NONE) {
    return named;
  }

  if (chars_ignoring_modifiers.length == 0) {
    return PX_KEY_NONE;
  }

  const unichar c = [chars_ignoring_modifiers characterAtIndex:0];
  if (is_private_use(c) || c < 0x20) {
    return PX_KEY_NONE;
  }

  // Unshifted, lowercased: the binding-table currency. Shift lives in the modifier mask, so
  // "shift+a" and "A" cannot disagree.
  if (c >= 'A' && c <= 'Z') {
    return static_cast<px_key>(c - 'A' + 'a');
  }
  return static_cast<px_key>(c);
}
