#include "experiments/gui_api_redesign/events/event.h"
#include "experiments/gui_api_redesign/events/event_utils.h"

namespace ui {

Event::Event(const PlatformEvent& native_event, EventType type, int flags)
    : type_(type), native_event_(native_event), flags_(flags) {}

KeyEvent* Event::as_key_event() { return static_cast<KeyEvent*>(this); }

const KeyEvent* Event::as_key_event() const { return static_cast<const KeyEvent*>(this); }

KeyEvent::KeyEvent(const PlatformEvent& native_event)
    : KeyEvent(native_event, EventFlagsFromNative(native_event)) {}

KeyEvent::KeyEvent(const PlatformEvent& native_event, int event_flags)
    : Event(native_event, EventTypeFromNative(native_event), event_flags),
      key_code_(KeyboardCodeFromNative(native_event)) {}

}  // namespace ui
