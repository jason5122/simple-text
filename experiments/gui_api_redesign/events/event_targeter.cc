#include "experiments/gui_api_redesign/events/event_targeter.h"

namespace ui {

EventTargeter::EventTargeter() = default;

EventTargeter::~EventTargeter() = default;

EventSink* EventTargeter::GetNewEventSinkForEvent(const EventTarget* current_root,
                                                  EventTarget* target,
                                                  Event* in_out_event) {
    return nullptr;
}

}  // namespace ui
