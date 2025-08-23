#pragma once

#include "experiments/gui_api_redesign/events/event_constants.h"
#include "experiments/gui_api_redesign/events/event_target.h"
#include "experiments/gui_api_redesign/events/event_type.h"
#include "experiments/gui_api_redesign/events/keycodes/keyboard_codes.h"
#include "experiments/gui_api_redesign/events/platform_event.h"

namespace ui {

class KeyEvent;

class Event {
public:
    EventType type() const { return type_; }
    const PlatformEvent& native_event() const { return native_event_; }
    int flags() const { return flags_; }
    EventTarget* target() const { return target_; }

    bool is_key_event() const {
        return type_ == EventType::kKeyPressed || type_ == EventType::kKeyReleased;
    }
    // Convenience methods to cast |this| to a KeyEvent. is_key_event() must be true as a
    // precondition to calling these methods.
    KeyEvent* as_key_event();
    const KeyEvent* as_key_event() const;

    bool handled() const { return result_ != ER_UNHANDLED; }

protected:
    Event(const PlatformEvent& native_event, EventType type, int flags);

private:
    EventType type_;
    PlatformEvent native_event_;
    int flags_;
    EventTarget* target_ = nullptr;
    EventResult result_ = ER_UNHANDLED;
};

class KeyEvent : public Event {
public:
    explicit KeyEvent(const PlatformEvent& native_event);
    KeyEvent(const PlatformEvent& native_event, int event_flags);

    KeyboardCode key_code() const { return key_code_; }

private:
    KeyboardCode key_code_;
};

}  // namespace ui
