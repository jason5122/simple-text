#include "experiments/gui_api_redesign/events/cocoa/cocoa_event_utils.h"
#include "experiments/gui_api_redesign/events/event_utils.h"
#include "experiments/gui_api_redesign/events/keycodes/keyboard_code_conversion_mac.h"
#include <spdlog/spdlog.h>

namespace ui {

EventType EventTypeFromNative(const PlatformEvent& platform_event) {
    NSEvent* event = platform_event.get();
    NSEventType type = event.type;
    switch (type) {
    // Standard types, handled.
    case NSEventTypeKeyDown:
    case NSEventTypeKeyUp:
    case NSEventTypeFlagsChanged:
        return IsKeyUpEvent(event) ? EventType::kKeyReleased : EventType::kKeyPressed;
    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
        return EventType::kMousePressed;
    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp:
        return EventType::kMouseReleased;
    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
        return EventType::kMouseDragged;
    case NSEventTypeMouseMoved:
        return EventType::kMouseMoved;
    case NSEventTypeScrollWheel:
        return EventType::kScroll;
    case NSEventTypeMouseEntered:
        return EventType::kMouseEntered;
    case NSEventTypeMouseExited:
        return EventType::kMouseExited;
    case NSEventTypeSwipe:
        return EventType::kScrollFlingStart;

    // Standard types, not handled.
    case NSEventTypeAppKitDefined:
    case NSEventTypeSystemDefined:
    case NSEventTypeApplicationDefined:
    case NSEventTypePeriodic:
    case NSEventTypeCursorUpdate:
    case NSEventTypeTabletPoint:
    case NSEventTypeTabletProximity:
    case NSEventTypeGesture:
    case NSEventTypeMagnify:
    case NSEventTypeRotate:
    case NSEventTypeBeginGesture:
    case NSEventTypeEndGesture:
    case NSEventTypeSmartMagnify:
    case NSEventTypeQuickLook:
    case NSEventTypePressure:
    case NSEventTypeDirectTouch:
    case NSEventTypeChangeMode:
        return EventType::kUnknown;

    // Non-standard types.
    case 36:
        // This is some kind of gesture event, seen during pinch-zooms, but seems
        // to be distinct from NSEventTypeMagnify. When sent the -description
        // message, it returns that it has type "Reserved", it has phases
        // (Began/Changed/Ended), and it has a "translation", printed as an
        // NSPoint, but with values from -deltaX and -deltaY.
        return EventType::kUnknown;

    default:
        spdlog::error("Not implemented: {}", event.debugDescription.UTF8String);
        return EventType::kUnknown;
    }
}

int EventFlagsFromNative(const PlatformEvent& platform_event) {
    NSEvent* event = platform_event.get();
    NSUInteger modifiers = [event modifierFlags];
    return EventFlagsFromNSEventWithModifiers(event, modifiers);
}

KeyboardCode KeyboardCodeFromNative(const PlatformEvent& platform_event) {
    return KeyboardCodeFromNSEvent(platform_event.get());
}

}  // namespace ui
