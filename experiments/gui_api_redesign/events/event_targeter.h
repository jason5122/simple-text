#pragma once

namespace ui {

class Event;
class EventSink;
class EventTarget;

class EventTargeter {
public:
    EventTargeter();
    virtual ~EventTargeter();
    EventTargeter(const EventTargeter&) = delete;
    EventTargeter& operator=(const EventTargeter&) = delete;

    // Returns the target |event| should be dispatched to. If there is no such
    // target, return NULL. If |event| is a located event, the location of |event|
    // is in the coordinate space of |root|. Furthermore, the targeter can mutate
    // the event (e.g., by changing the location of the event to be in the
    // returned target's coordinate space) so that it can be dispatched to the
    // target without any further modification.
    virtual EventTarget* FindTargetForEvent(EventTarget* root, Event* event) = 0;

    // Returns the next best target for |event| as compared to |previous_target|.
    // |event| is in the local coordinate space of |previous_target|.
    // Also mutates |event| so that it can be dispatched to the returned target
    // (e.g., by changing |event|'s location to be in the returned target's
    // coordinate space).
    virtual EventTarget* FindNextBestTarget(EventTarget* previous_target, Event* event) = 0;

    // Returns new event sink if the `in_out_event` should be dispatched to a
    // different sink. The event will be updated so that it can be dispatched to
    // the new sink correctly. Returns `nullptr` if the event do not have to be
    // redirected.
    virtual EventSink* GetNewEventSinkForEvent(const EventTarget* current_root,
                                               EventTarget* target,
                                               Event* in_out_event);
};

}  // namespace ui
