#include "experiments/message_loop/message_pump_mac.h"
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

using base::apple::OwnershipPolicy;

namespace {
constexpr CFTimeInterval kCFTimeIntervalMax = std::numeric_limits<CFTimeInterval>::max();
}

// =================================================================================================
// MessagePumpCFRunLoopBase
// =================================================================================================
MessagePumpCFRunLoopBase::MessagePumpCFRunLoopBase() {
    run_loop_.reset(CFRunLoopGetCurrent(), OwnershipPolicy::kRetain);

    // Set a repeating timer with a preposterous firing time and interval. The timer will
    // effectively never fire as-is.  The firing time will be adjusted as needed when
    // `schedule_delayed_work` is called.
    CFRunLoopTimerContext timer_context = {0};
    timer_context.info = this;
    delayed_work_timer_.reset(CFRunLoopTimerCreate(/*allocator=*/nullptr,
                                                   /*fireDate=*/kCFTimeIntervalMax,
                                                   /*interval=*/kCFTimeIntervalMax,
                                                   /*flags=*/0,
                                                   /*order=*/0,
                                                   /*callout=*/run_delayed_work_timer,
                                                   /*context=*/&timer_context));

    CFRunLoopSourceContext source_context = {0};
    source_context.info = this;
    source_context.perform = run_work_source;
    work_source_.reset(CFRunLoopSourceCreate(/*allocator=*/nullptr,
                                             /*order=*/1,
                                             /*context=*/&source_context));

    CFRunLoopAddTimer(run_loop_.get(), delayed_work_timer_.get(), kCFRunLoopCommonModes);
    CFRunLoopAddSource(run_loop_.get(), work_source_.get(), kCFRunLoopCommonModes);
}

MessagePumpCFRunLoopBase::~MessagePumpCFRunLoopBase() {
    CFRunLoopRemoveSource(run_loop_.get(), work_source_.get(), kCFRunLoopCommonModes);
    CFRunLoopRemoveTimer(run_loop_.get(), delayed_work_timer_.get(), kCFRunLoopCommonModes);
}

void MessagePumpCFRunLoopBase::run(Delegate* delegate) {
    delegate_ = delegate;

    schedule_work();
    do_run(delegate);
}

void MessagePumpCFRunLoopBase::quit() { do_quit(); }

void MessagePumpCFRunLoopBase::schedule_work() {
    // Schedule work. May be called on any thread.
    CFRunLoopSourceSignal(work_source_.get());
    CFRunLoopWakeUp(run_loop_.get());
}

void MessagePumpCFRunLoopBase::schedule_delayed_work(
    const Delegate::NextWorkInfo& next_work_info) {
    if (next_work_info.leeway != delayed_work_leeway_) {
        if (!next_work_info.leeway.is_zero()) {
            CFRunLoopTimerSetTolerance(delayed_work_timer_.get(),
                                       next_work_info.leeway.in_seconds_f());
        } else {
            CFRunLoopTimerSetTolerance(delayed_work_timer_.get(), 0);
        }
        delayed_work_leeway_ = next_work_info.leeway;
    }

    if (next_work_info.delayed_run_time != delayed_work_scheduled_at_) {
        if (next_work_info.delayed_run_time.is_max()) {
            CFRunLoopTimerSetNextFireDate(delayed_work_timer_.get(), kCFTimeIntervalMax);
        } else {
            const double delay_seconds = next_work_info.remaining_delay().in_seconds_f();
            CFRunLoopTimerSetNextFireDate(delayed_work_timer_.get(),
                                          CFAbsoluteTimeGetCurrent() + delay_seconds);
        }

        delayed_work_scheduled_at_ = next_work_info.delayed_run_time;
    }
}

void MessagePumpCFRunLoopBase::run_delayed_work_timer(CFRunLoopTimerRef timer, void* info) {
    auto* self = static_cast<MessagePumpCFRunLoopBase*>(info);
    self->delayed_work_scheduled_at_ = base::TimeTicks::max();
    self->run_work();
}

void MessagePumpCFRunLoopBase::run_work_source(void* info) {
    auto* self = static_cast<MessagePumpCFRunLoopBase*>(info);
    self->run_work();
}

bool MessagePumpCFRunLoopBase::run_work() {
    auto next_work_info = delegate_->do_work();

    if (next_work_info.is_immediate()) {
        CFRunLoopSourceSignal(work_source_.get());
        return true;
    } else {
        schedule_delayed_work(next_work_info);
        return false;
    }
}

// =================================================================================================
// MessagePumpNSApplication
// =================================================================================================
namespace {
// The MessagePump controlling [NSApp run].
MessagePumpNSApplication* g_app_pump;
}  // namespace

MessagePumpNSApplication::MessagePumpNSApplication() { g_app_pump = this; }

MessagePumpNSApplication::~MessagePumpNSApplication() { g_app_pump = nullptr; }

void MessagePumpNSApplication::do_run(Delegate* delegate) { [NSApp run]; }

bool MessagePumpNSApplication::do_quit() {
    [NSApp stop:nil];

    // Send a fake event to wake the loop up.
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSZeroPoint
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0]
             atStart:NO];

    return true;
}

namespace message_pump_mac {

std::unique_ptr<MessagePump> create() {
    // Executables which have specific requirements for their NSApplication subclass should
    // initialize appropriately before creating an event loop.
    [NSApplication sharedApplication];
    return std::make_unique<MessagePumpNSApplication>();
}

}  // namespace message_pump_mac
