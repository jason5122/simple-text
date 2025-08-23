#pragma once

#include "experiments/gui_api_redesign/events/event_handler.h"
#include <memory>

namespace ui {

class Event;
class EventTargetIterator;
class EventTargeter;

class EventTarget {
public:
    EventTarget();
    virtual ~EventTarget();
    EventTarget(const EventTarget&) = delete;
    EventTarget& operator=(const EventTarget&) = delete;

    virtual bool can_accept_event(const Event& event) = 0;

    // Returns the parent EventTarget in the event-target tree.
    virtual EventTarget* get_parent_target() = 0;

    // Returns an iterator an EventTargeter can use to iterate over the list of
    // child EventTargets.
    virtual std::unique_ptr<EventTargetIterator> get_child_iterator() const = 0;

    // Returns the EventTargeter that should be used to find the target for an
    // event in the subtree rooted at this EventTarget.
    virtual EventTargeter* GetEventTargeter() = 0;

    // Sets |target_handler| as |target_handler_| and returns the old handler.
    EventHandler* set_target_handler(EventHandler* target_handler);

protected:
    EventHandler* target_handler() { return target_handler_; }

    EventHandler* target_handler_ = nullptr;
};

}  // namespace ui
