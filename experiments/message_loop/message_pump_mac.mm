#include "experiments/message_loop/message_pump_mac.h"
#include <AppKit/AppKit.h>

namespace base {

std::unique_ptr<MessagePump> MessagePump::create_for_ui() {
    [NSApplication sharedApplication];
    return std::make_unique<MessagePumpNSApplication>();
}

MessagePumpNSApplication::MessagePumpNSApplication() {}

MessagePumpNSApplication::~MessagePumpNSApplication() {}

void MessagePumpNSApplication::run() {
    schedule_work();
    [NSApp run];
}

void MessagePumpNSApplication::quit() { [NSApp stop:nil]; }

void MessagePumpNSApplication::schedule_work() {
    // CFRunLoopSourceSignal(work_source_.get());
    CFRunLoopWakeUp(run_loop_.get());
}

}  // namespace base
