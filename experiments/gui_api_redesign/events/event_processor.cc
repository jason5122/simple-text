#include "experiments/gui_api_redesign/events/event.h"
#include "experiments/gui_api_redesign/events/event_processor.h"
#include "experiments/gui_api_redesign/events/event_target.h"

namespace ui {

EventProcessor::EventProcessor() = default;

EventProcessor::~EventProcessor() = default;

EventDispatchDetails EventProcessor::OnEventFromSource(Event* event) {
    EventDispatchDetails details;
    EventTarget* target = nullptr;
    if (!event->handled()) {
        EventTarget* root = GetRootForEvent(event);
    }
    return details;
}

}  // namespace ui
