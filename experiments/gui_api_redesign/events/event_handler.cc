#include "experiments/gui_api_redesign/events/event.h"
#include "experiments/gui_api_redesign/events/event_handler.h"

namespace ui {

EventHandler::EventHandler() = default;

EventHandler::~EventHandler() = default;

void EventHandler::on_event(Event* event) {
    if (event->is_key_event()) on_key_event(event->as_key_event());
}

void EventHandler::on_key_event(KeyEvent* event) {}

}  // namespace ui
