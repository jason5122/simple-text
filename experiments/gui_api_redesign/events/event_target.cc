#include "experiments/gui_api_redesign/events/event_target.h"

namespace ui {

EventTarget::EventTarget() = default;

EventTarget::~EventTarget() = default;

EventHandler* EventTarget::set_target_handler(EventHandler* target_handler) {
    EventHandler* original_target_handler = target_handler_;
    target_handler_ = target_handler;
    return original_target_handler;
}

}  // namespace ui
