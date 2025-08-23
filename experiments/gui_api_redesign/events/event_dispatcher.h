#pragma once

namespace ui {

struct EventDispatchDetails {
    bool dispatcher_destroyed = false;
    bool target_destroyed = false;

    // Set to true if an EventRewriter discards the event.
    bool event_discarded = false;
};

}  // namespace ui
