#pragma once

#include "experiments/gui_api_redesign/events/keycodes/keyboard_codes_posix.h"
#include <Carbon/Carbon.h>
#include <Cocoa/Cocoa.h>

namespace ui {

// Returns the WindowsKeyCode from the Mac key code.
KeyboardCode KeyboardCodeFromKeyCode(unsigned short key_code);

// Returns the KeyboardCode from a |char_code| from AppKit classes.
KeyboardCode KeyboardCodeFromCharCode(unichar char_code);

KeyboardCode KeyboardCodeFromNSEvent(NSEvent* event);

}  // namespace ui
