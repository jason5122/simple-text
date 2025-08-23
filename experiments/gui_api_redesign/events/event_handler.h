#pragma once

namespace ui {

class Event;
class KeyEvent;

class EventHandler {
public:
    EventHandler();
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    virtual ~EventHandler();

    // This is called for all events. The default implementation routes the event
    // to one of the event-specific callbacks (OnKeyEvent, OnMouseEvent etc.). If
    // this is overridden, then normally, the override should chain into the
    // default implementation for un-handled events.
    virtual void on_event(Event* event);

    virtual void on_key_event(KeyEvent* event);
};

}  // namespace ui
