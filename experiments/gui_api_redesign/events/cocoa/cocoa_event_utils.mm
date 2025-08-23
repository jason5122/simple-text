#include "experiments/gui_api_redesign/events/cocoa/cocoa_event_utils.h"
#include "experiments/gui_api_redesign/events/event_constants.h"
#include <Carbon/Carbon.h>

namespace {

bool IsLeftButtonEvent(NSEvent* event) {
    NSEventType type = event.type;
    return type == NSEventTypeLeftMouseDown || type == NSEventTypeLeftMouseDragged ||
           type == NSEventTypeLeftMouseUp;
}

bool IsRightButtonEvent(NSEvent* event) {
    NSEventType type = event.type;
    return type == NSEventTypeRightMouseDown || type == NSEventTypeRightMouseDragged ||
           type == NSEventTypeRightMouseUp;
}

bool IsMiddleButtonEvent(NSEvent* event) {
    if (event.buttonNumber != 2) {
        return false;
    }

    NSEventType type = [event type];
    return type == NSEventTypeOtherMouseDown || type == NSEventTypeOtherMouseDragged ||
           type == NSEventTypeOtherMouseUp;
}

// Return true if the target modifier key is up. OS X has an "official" flag to test whether either
// left or right versions of a modifier key are held, and "unofficial" flags for the left and right
// versions independently. This function verifies that |target_key_mask| and |otherKeyMask| (which
// should be the left and right versions of a modifier) are consistent with with the state of
// |eitherKeyMask| (which should be the corresponding ""official" flag). If they are consistent, it
// tests |target_key_mask|; otherwise it tests |either_key_mask|.
inline bool IsModifierKeyUp(unsigned int flags,
                            unsigned int target_key_mask,
                            unsigned int other_key_mask,
                            unsigned int either_key_mask) {
    bool either_key_down = (flags & either_key_mask) != 0;
    bool target_key_down = (flags & target_key_mask) != 0;
    bool other_key_down = (flags & other_key_mask) != 0;
    if (either_key_down != (target_key_down || other_key_down)) return !either_key_down;
    return !target_key_down;
}

}  // namespace

namespace ui {

int EventFlagsFromModifiers(NSUInteger modifiers) {
    int flags = 0;
    flags |= (modifiers & NSEventModifierFlagCapsLock) ? ui::EF_CAPS_LOCK_ON : 0;
    flags |= (modifiers & NSEventModifierFlagShift) ? ui::EF_SHIFT_DOWN : 0;
    flags |= (modifiers & NSEventModifierFlagControl) ? ui::EF_CONTROL_DOWN : 0;
    flags |= (modifiers & NSEventModifierFlagOption) ? ui::EF_ALT_DOWN : 0;
    flags |= (modifiers & NSEventModifierFlagCommand) ? ui::EF_COMMAND_DOWN : 0;
    return flags;
}

int EventFlagsFromNSEventWithModifiers(NSEvent* event, NSUInteger modifiers) {
    int flags = EventFlagsFromModifiers(modifiers);
    if (IsLeftButtonEvent(event)) {
        // For Mac, convert Ctrl+LeftClick to a RightClick, and remove the Control
        // key modifier.
        if (modifiers & NSEventModifierFlagControl)
            flags = (flags & ~ui::EF_CONTROL_DOWN) | ui::EF_RIGHT_MOUSE_BUTTON;
        else flags |= ui::EF_LEFT_MOUSE_BUTTON;
    }

    flags |= IsRightButtonEvent(event) ? ui::EF_RIGHT_MOUSE_BUTTON : 0;
    flags |= IsMiddleButtonEvent(event) ? ui::EF_MIDDLE_MOUSE_BUTTON : 0;

    if (event.type == NSEventTypeKeyDown && event.ARepeat) {
        flags |= ui::EF_IS_REPEAT;
    }

    return flags;
}

bool IsKeyUpEvent(NSEvent* event) {
    if (event.type != NSEventTypeFlagsChanged) {
        return event.type == NSEventTypeKeyUp;
    }

    switch (event.keyCode) {
    case kVK_Command:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICELCMDKEYMASK, NX_DEVICERCMDKEYMASK,
                               NSEventModifierFlagCommand);
    case kVK_RightCommand:
        return IsModifierKeyUp([event modifierFlags], NX_DEVICERCMDKEYMASK, NX_DEVICELCMDKEYMASK,
                               NSEventModifierFlagCommand);

    case kVK_CapsLock:
        return (event.modifierFlags & NSEventModifierFlagCapsLock) == 0;

    case kVK_Shift:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICELSHIFTKEYMASK, NX_DEVICERSHIFTKEYMASK,
                               NSEventModifierFlagShift);
    case kVK_RightShift:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICERSHIFTKEYMASK, NX_DEVICELSHIFTKEYMASK,
                               NSEventModifierFlagShift);

    case kVK_Option:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICELALTKEYMASK, NX_DEVICERALTKEYMASK,
                               NSEventModifierFlagOption);
    case kVK_RightOption:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICERALTKEYMASK, NX_DEVICELALTKEYMASK,
                               NSEventModifierFlagOption);

    case kVK_Control:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICELCTLKEYMASK, NX_DEVICERCTLKEYMASK,
                               NSEventModifierFlagControl);
    case kVK_RightControl:
        return IsModifierKeyUp(event.modifierFlags, NX_DEVICERCTLKEYMASK, NX_DEVICELCTLKEYMASK,
                               NSEventModifierFlagControl);

    case kVK_Function:
        return (event.modifierFlags & NSEventModifierFlagFunction) == 0;
    }
    return false;
}

}  // namespace ui
