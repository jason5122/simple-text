#pragma once

#include "experiments/gui_api_redesign/events/event_type.h"
#include "experiments/gui_api_redesign/events/keycodes/keyboard_codes.h"
#include "experiments/gui_api_redesign/events/platform_event.h"

namespace ui {

EventType EventTypeFromNative(const PlatformEvent& native_event);

int EventFlagsFromNative(const PlatformEvent& native_event);

KeyboardCode KeyboardCodeFromNative(const PlatformEvent& native_event);

}  // namespace ui
