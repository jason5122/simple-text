#pragma once

namespace ui {

class EventSink;

class EventSource {
public:
    EventSource();
    virtual ~EventSource();
    EventSource(const EventSource&) = delete;
    EventSource& operator=(const EventSource&) = delete;

    virtual EventSink* GetEventSink() = 0;
};

}  // namespace ui
