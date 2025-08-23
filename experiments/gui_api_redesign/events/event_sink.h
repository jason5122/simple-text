#pragma once

#include "experiments/gui_api_redesign/events/event_dispatcher.h"

namespace ui {

class Event;

// EventSink receives events from an EventSource.
class EventSink {
public:
    virtual ~EventSink() {}

    // Receives events from EventSource.
    [[nodiscard]] virtual EventDispatchDetails OnEventFromSource(Event* event) = 0;
};

}  // namespace ui
