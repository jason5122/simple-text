#pragma once

#include "experiments/gui_api_redesign/events/event_sink.h"

namespace ui {

class EventTarget;
class EventTargeter;

class EventProcessor : public EventSink {
public:
    EventProcessor();
    ~EventProcessor() override;
    EventProcessor(const EventProcessor&) = delete;
    EventProcessor& operator=(const EventProcessor&) = delete;

    // EventSink overrides:
    EventDispatchDetails OnEventFromSource(Event* event) override;

    // Returns the EventTarget with the right EventTargeter that we should use for
    // dispatching this |event|.
    virtual EventTarget* GetRootForEvent(Event* event) = 0;

    // If the root target returned by GetRootForEvent() does not have a
    // targeter set, then the default targeter is used to find the target.
    virtual EventTargeter* GetDefaultEventTargeter() = 0;
};

}  // namespace ui
