#include "base/containers/fixed_flat_map.h"
#include "experiments/gui_api_redesign/events/keycodes/keyboard_code_conversion_mac.h"
#include <Carbon/Carbon.h>

namespace {

bool IsKeypadOrNumericKeyEvent(NSEvent* event) {
    // Check that this is the type of event that has a keyCode.
    switch (event.type) {
    case NSEventTypeKeyDown:
    case NSEventTypeKeyUp:
    case NSEventTypeFlagsChanged:
        break;
    default:
        return false;
    }

    switch (event.keyCode) {
    case kVK_ANSI_KeypadClear:
    case kVK_ANSI_KeypadEquals:
    case kVK_ANSI_KeypadMultiply:
    case kVK_ANSI_KeypadDivide:
    case kVK_ANSI_KeypadMinus:
    case kVK_ANSI_KeypadPlus:
    case kVK_ANSI_KeypadEnter:
    case kVK_ANSI_KeypadDecimal:
    case kVK_ANSI_Keypad0:
    case kVK_ANSI_Keypad1:
    case kVK_ANSI_Keypad2:
    case kVK_ANSI_Keypad3:
    case kVK_ANSI_Keypad4:
    case kVK_ANSI_Keypad5:
    case kVK_ANSI_Keypad6:
    case kVK_ANSI_Keypad7:
    case kVK_ANSI_Keypad8:
    case kVK_ANSI_Keypad9:
    case kVK_ANSI_0:
    case kVK_ANSI_1:
    case kVK_ANSI_2:
    case kVK_ANSI_3:
    case kVK_ANSI_4:
    case kVK_ANSI_5:
    case kVK_ANSI_6:
    case kVK_ANSI_7:
    case kVK_ANSI_8:
    case kVK_ANSI_9:
        return true;
    }

    return false;
}

}  // namespace

namespace ui {

KeyboardCode KeyboardCodeFromCharCode(unichar char_code) {
    constexpr auto kMap = base::MakeFixedFlatMap<unichar, KeyboardCode>({
        {'a', VKEY_A},
        {'A', VKEY_A},
        {'b', VKEY_B},
        {'B', VKEY_B},
        {'c', VKEY_C},
        {'C', VKEY_C},
        {'d', VKEY_D},
        {'D', VKEY_D},
        {'e', VKEY_E},
        {'E', VKEY_E},
        {'f', VKEY_F},
        {'F', VKEY_F},
        {'g', VKEY_G},
        {'G', VKEY_G},
        {'h', VKEY_H},
        {'H', VKEY_H},
        {'i', VKEY_I},
        {'I', VKEY_I},
        {'j', VKEY_J},
        {'J', VKEY_J},
        {'k', VKEY_K},
        {'K', VKEY_K},
        {'l', VKEY_L},
        {'L', VKEY_L},
        {'m', VKEY_M},
        {'M', VKEY_M},
        {'n', VKEY_N},
        {'N', VKEY_N},
        {'o', VKEY_O},
        {'O', VKEY_O},
        {'p', VKEY_P},
        {'P', VKEY_P},
        {'q', VKEY_Q},
        {'Q', VKEY_Q},
        {'r', VKEY_R},
        {'R', VKEY_R},
        {'s', VKEY_S},
        {'S', VKEY_S},
        {'t', VKEY_T},
        {'T', VKEY_T},
        {'u', VKEY_U},
        {'U', VKEY_U},
        {'v', VKEY_V},
        {'V', VKEY_V},
        {'w', VKEY_W},
        {'W', VKEY_W},
        {'x', VKEY_X},
        {'X', VKEY_X},
        {'y', VKEY_Y},
        {'Y', VKEY_Y},
        {'z', VKEY_Z},
        {'Z', VKEY_Z},

        {'1', VKEY_1},
        {'2', VKEY_2},
        {'3', VKEY_3},
        {'4', VKEY_4},
        {'5', VKEY_5},
        {'6', VKEY_6},
        {'7', VKEY_7},
        {'8', VKEY_8},
        {'9', VKEY_9},
        {'0', VKEY_0},

        {NSPauseFunctionKey, VKEY_PAUSE},
        {NSSelectFunctionKey, VKEY_SELECT},
        {NSPrintFunctionKey, VKEY_PRINT},
        {NSExecuteFunctionKey, VKEY_EXECUTE},
        {NSPrintScreenFunctionKey, VKEY_SNAPSHOT},
        {NSInsertFunctionKey, VKEY_INSERT},
        {NSF21FunctionKey, VKEY_F21},
        {NSF22FunctionKey, VKEY_F22},
        {NSF23FunctionKey, VKEY_F23},
        {NSF24FunctionKey, VKEY_F24},
        {NSScrollLockFunctionKey, VKEY_SCROLL},

        // U.S. Specific mappings.  Mileage may vary.
        {';', VKEY_OEM_1},
        {':', VKEY_OEM_1},
        {'=', VKEY_OEM_PLUS},
        {'+', VKEY_OEM_PLUS},
        {',', VKEY_OEM_COMMA},
        {'<', VKEY_OEM_COMMA},
        {'-', VKEY_OEM_MINUS},
        {'_', VKEY_OEM_MINUS},
        {'.', VKEY_OEM_PERIOD},
        {'>', VKEY_OEM_PERIOD},
        {'/', VKEY_OEM_2},
        {'?', VKEY_OEM_2},
        {'`', VKEY_OEM_3},
        {'~', VKEY_OEM_3},
        {'[', VKEY_OEM_4},
        {'{', VKEY_OEM_4},
        {'\\', VKEY_OEM_5},
        {'|', VKEY_OEM_5},
        {']', VKEY_OEM_6},
        {'}', VKEY_OEM_6},
        {'\'', VKEY_OEM_7},
        {'"', VKEY_OEM_7},
    });

    auto it = kMap.find(char_code);
    if (it != kMap.end()) {
        return it->second;
    }

    return VKEY_UNKNOWN;
}

KeyboardCode KeyboardCodeFromKeyCode(unsigned short key_code) {
    static const KeyboardCode kKeyboardCodes[] = {
        /* 0x00 */ VKEY_A,
        /* 0x01 */ VKEY_S,
        /* 0x02 */ VKEY_D,
        /* 0x03 */ VKEY_F,
        /* 0x04 */ VKEY_H,
        /* 0x05 */ VKEY_G,
        /* 0x06 */ VKEY_Z,
        /* 0x07 */ VKEY_X,
        /* 0x08 */ VKEY_C,
        /* 0x09 */ VKEY_V,
        /* 0x0A */ VKEY_OEM_3,  // Section key.
        /* 0x0B */ VKEY_B,
        /* 0x0C */ VKEY_Q,
        /* 0x0D */ VKEY_W,
        /* 0x0E */ VKEY_E,
        /* 0x0F */ VKEY_R,
        /* 0x10 */ VKEY_Y,
        /* 0x11 */ VKEY_T,
        /* 0x12 */ VKEY_1,
        /* 0x13 */ VKEY_2,
        /* 0x14 */ VKEY_3,
        /* 0x15 */ VKEY_4,
        /* 0x16 */ VKEY_6,
        /* 0x17 */ VKEY_5,
        /* 0x18 */ VKEY_OEM_PLUS,  // =+
        /* 0x19 */ VKEY_9,
        /* 0x1A */ VKEY_7,
        /* 0x1B */ VKEY_OEM_MINUS,  // -_
        /* 0x1C */ VKEY_8,
        /* 0x1D */ VKEY_0,
        /* 0x1E */ VKEY_OEM_6,  // ]}
        /* 0x1F */ VKEY_O,
        /* 0x20 */ VKEY_U,
        /* 0x21 */ VKEY_OEM_4,  // {[
        /* 0x22 */ VKEY_I,
        /* 0x23 */ VKEY_P,
        /* 0x24 */ VKEY_RETURN,  // Return
        /* 0x25 */ VKEY_L,
        /* 0x26 */ VKEY_J,
        /* 0x27 */ VKEY_OEM_7,  // '"
        /* 0x28 */ VKEY_K,
        /* 0x29 */ VKEY_OEM_1,      // ;:
        /* 0x2A */ VKEY_OEM_5,      // \|
        /* 0x2B */ VKEY_OEM_COMMA,  // ,<
        /* 0x2C */ VKEY_OEM_2,      // /?
        /* 0x2D */ VKEY_N,
        /* 0x2E */ VKEY_M,
        /* 0x2F */ VKEY_OEM_PERIOD,  // .>
        /* 0x30 */ VKEY_TAB,
        /* 0x31 */ VKEY_SPACE,
        /* 0x32 */ VKEY_OEM_3,    // `~
        /* 0x33 */ VKEY_BACK,     // Backspace
        /* 0x34 */ VKEY_UNKNOWN,  // n/a
        /* 0x35 */ VKEY_ESCAPE,
        /* 0x36 */ VKEY_APPS,     // Right Command
        /* 0x37 */ VKEY_LWIN,     // Left Command
        /* 0x38 */ VKEY_SHIFT,    // Left Shift
        /* 0x39 */ VKEY_CAPITAL,  // Caps Lock
        /* 0x3A */ VKEY_MENU,     // Left Option
        /* 0x3B */ VKEY_CONTROL,  // Left Ctrl
        /* 0x3C */ VKEY_SHIFT,    // Right Shift
        /* 0x3D */ VKEY_MENU,     // Right Option
        /* 0x3E */ VKEY_CONTROL,  // Right Ctrl
        /* 0x3F */ VKEY_UNKNOWN,  // fn
        /* 0x40 */ VKEY_F17,
        /* 0x41 */ VKEY_DECIMAL,   // Num Pad .
        /* 0x42 */ VKEY_UNKNOWN,   // n/a
        /* 0x43 */ VKEY_MULTIPLY,  // Num Pad *
        /* 0x44 */ VKEY_UNKNOWN,   // n/a
        /* 0x45 */ VKEY_ADD,       // Num Pad +
        /* 0x46 */ VKEY_UNKNOWN,   // n/a
        /* 0x47 */ VKEY_CLEAR,     // Num Pad Clear
        /* 0x48 */ VKEY_VOLUME_UP,
        /* 0x49 */ VKEY_VOLUME_DOWN,
        /* 0x4A */ VKEY_VOLUME_MUTE,
        /* 0x4B */ VKEY_DIVIDE,    // Num Pad /
        /* 0x4C */ VKEY_RETURN,    // Num Pad Enter
        /* 0x4D */ VKEY_UNKNOWN,   // n/a
        /* 0x4E */ VKEY_SUBTRACT,  // Num Pad -
        /* 0x4F */ VKEY_F18,
        /* 0x50 */ VKEY_F19,
        /* 0x51 */ VKEY_OEM_PLUS,  // Num Pad =.
        /* 0x52 */ VKEY_NUMPAD0,
        /* 0x53 */ VKEY_NUMPAD1,
        /* 0x54 */ VKEY_NUMPAD2,
        /* 0x55 */ VKEY_NUMPAD3,
        /* 0x56 */ VKEY_NUMPAD4,
        /* 0x57 */ VKEY_NUMPAD5,
        /* 0x58 */ VKEY_NUMPAD6,
        /* 0x59 */ VKEY_NUMPAD7,
        /* 0x5A */ VKEY_F20,
        /* 0x5B */ VKEY_NUMPAD8,
        /* 0x5C */ VKEY_NUMPAD9,
        /* 0x5D */ VKEY_UNKNOWN,  // Yen (JIS Keyboard Only)
        /* 0x5E */ VKEY_UNKNOWN,  // Underscore (JIS Keyboard Only)
        /* 0x5F */ VKEY_UNKNOWN,  // KeypadComma (JIS Keyboard Only)
        /* 0x60 */ VKEY_F5,
        /* 0x61 */ VKEY_F6,
        /* 0x62 */ VKEY_F7,
        /* 0x63 */ VKEY_F3,
        /* 0x64 */ VKEY_F8,
        /* 0x65 */ VKEY_F9,
        /* 0x66 */ VKEY_UNKNOWN,  // Eisu (JIS Keyboard Only)
        /* 0x67 */ VKEY_F11,
        /* 0x68 */ VKEY_UNKNOWN,  // Kana (JIS Keyboard Only)
        /* 0x69 */ VKEY_F13,
        /* 0x6A */ VKEY_F16,
        /* 0x6B */ VKEY_F14,
        /* 0x6C */ VKEY_UNKNOWN,  // n/a
        /* 0x6D */ VKEY_F10,
        /* 0x6E */ VKEY_APPS,  // Context Menu key
        /* 0x6F */ VKEY_F12,
        /* 0x70 */ VKEY_UNKNOWN,  // n/a
        /* 0x71 */ VKEY_F15,
        /* 0x72 */ VKEY_INSERT,  // Help
        /* 0x73 */ VKEY_HOME,    // Home
        /* 0x74 */ VKEY_PRIOR,   // Page Up
        /* 0x75 */ VKEY_DELETE,  // Forward Delete
        /* 0x76 */ VKEY_F4,
        /* 0x77 */ VKEY_END,  // End
        /* 0x78 */ VKEY_F2,
        /* 0x79 */ VKEY_NEXT,  // Page Down
        /* 0x7A */ VKEY_F1,
        /* 0x7B */ VKEY_LEFT,    // Left Arrow
        /* 0x7C */ VKEY_RIGHT,   // Right Arrow
        /* 0x7D */ VKEY_DOWN,    // Down Arrow
        /* 0x7E */ VKEY_UP,      // Up Arrow
        /* 0x7F */ VKEY_UNKNOWN  // n/a
    };

    if (key_code >= 0x80) {
        return VKEY_UNKNOWN;
    }

    return kKeyboardCodes[key_code];
}

KeyboardCode KeyboardCodeFromNSEvent(NSEvent* event) {
    KeyboardCode code = VKEY_UNKNOWN;

    // Numeric keys 0-9 should always return |keyCode| 0-9.
    // https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/keyCode#Printable_keys_in_standard_position
    if (!IsKeypadOrNumericKeyEvent(event) &&
        (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp)) {
        // Handles Dvorak-QWERTY Cmd case.
        // https://github.com/WebKit/webkit/blob/4d41c98b1de467f5d2a8fcba84d7c5268f11b0cc/Source/WebCore/platform/mac/PlatformEventFactoryMac.mm#L329
        NSString* characters = event.characters;
        if (characters.length > 0) {
            code = KeyboardCodeFromCharCode([characters characterAtIndex:0]);
        }
        if (code) {
            return code;
        }

        characters = event.charactersIgnoringModifiers;
        if (characters.length > 0) {
            code = KeyboardCodeFromCharCode([characters characterAtIndex:0]);
        }
        if (code) {
            return code;
        }
    }
    return KeyboardCodeFromKeyCode(event.keyCode);
}

}  // namespace ui
