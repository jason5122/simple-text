#pragma once

#include "base/apple/scoped_cftyperef.h"
#include "experiments/message_loop/message_pump.h"
#include <CoreFoundation/CoreFoundation.h>

namespace base {

class MessagePumpNSApplication : public MessagePump {
public:
    MessagePumpNSApplication();
    ~MessagePumpNSApplication() override;
    MessagePumpNSApplication(const MessagePumpNSApplication&) = delete;
    MessagePumpNSApplication& operator=(const MessagePumpNSApplication&) = delete;

    void run() override;
    void quit() override;
    void schedule_work() override;

private:
    apple::ScopedCFTypeRef<CFRunLoopRef> run_loop_;
};

}  // namespace base
