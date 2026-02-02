#pragma once

#include "base/apple/scoped_cftyperef.h"
#include "experiments/message_loop/message_pump.h"
#include <CoreFoundation/CoreFoundation.h>
#include <memory>

class MessagePumpCFRunLoopBase : public MessagePump {
public:
    MessagePumpCFRunLoopBase(const MessagePumpCFRunLoopBase&) = delete;
    MessagePumpCFRunLoopBase& operator=(const MessagePumpCFRunLoopBase&) = delete;

    void run(Delegate* delegate) override;
    void quit() override;
    void schedule_work() override;
    void schedule_delayed_work(const Delegate::NextWorkInfo& next_work_info) override;

protected:
    MessagePumpCFRunLoopBase();
    virtual ~MessagePumpCFRunLoopBase();

    virtual void do_run(Delegate* delegate) = 0;
    virtual bool do_quit() = 0;

    CFRunLoopRef run_loop() const { return run_loop_.get(); }

private:
    static void run_delayed_work_timer(CFRunLoopTimerRef timer, void* info);
    static void run_work_source(void* info);
    bool run_work();

    base::apple::ScopedCFTypeRef<CFRunLoopRef> run_loop_;
    base::apple::ScopedCFTypeRef<CFRunLoopTimerRef> delayed_work_timer_;
    base::apple::ScopedCFTypeRef<CFRunLoopSourceRef> work_source_;
    Delegate* delegate_ = nullptr;

    // Time at which `delayed_work_timer_` is set to fire.
    base::TimeTicks delayed_work_scheduled_at_ = base::TimeTicks::max();
    base::TimeDelta delayed_work_leeway_;
};

class MessagePumpNSApplication : public MessagePumpCFRunLoopBase {
public:
    MessagePumpNSApplication();
    ~MessagePumpNSApplication() override;
    MessagePumpNSApplication(const MessagePumpNSApplication&) = delete;
    MessagePumpNSApplication& operator=(const MessagePumpNSApplication&) = delete;

    void do_run(Delegate* delegate) override;
    bool do_quit() override;
};

namespace message_pump_mac {
std::unique_ptr<MessagePump> create();
}
