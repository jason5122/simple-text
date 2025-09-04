#pragma once

#include "build/build_config.h"
#include <memory>

namespace base {

class MessagePump {
public:
    static std::unique_ptr<MessagePump> create_for_ui();
    MessagePump() = default;
    virtual ~MessagePump() = default;

    virtual void run() = 0;
    virtual void quit() = 0;
    virtual void schedule_work() = 0;
};

}  // namespace base
