#pragma once

#include "base/time/time.h"
#include "build/build_config.h"
#include <memory>

namespace base {

#if BUILDFLAG(IS_WIN)
constexpr TimeDelta kDefaultLeeway = TimeDelta::milliseconds(16);
#else
constexpr TimeDelta kDefaultLeeway = TimeDelta::milliseconds(8);
#endif  // #if !BUILDFLAG(IS_WIN)

class MessagePump {
public:
    static std::unique_ptr<MessagePump> create();

    class Delegate {
    public:
        virtual ~Delegate() = default;

        struct NextWorkInfo {
            // Helper to verify if the next task is ready right away.
            bool is_immediate() const { return delayed_run_time.is_null(); }

            TimeDelta remaining_delay() const { return delayed_run_time - TimeTicks::now(); }

            // The next PendingTask's |delayed_run_time|. is_null() if there's extra work to run
            // immediately. is_max() if there are no more immediate nor delayed tasks.
            TimeTicks delayed_run_time;

            // |leeway| determines the preferred time range for scheduling work. A larger leeway
            // provides more freedom to schedule work at
            // an optimal time for power consumption. This field is ignored
            // for immediate work.
            TimeDelta leeway = kDefaultLeeway;
        };

        virtual NextWorkInfo do_work() = 0;
    };

    MessagePump();
    virtual ~MessagePump();

    virtual void run(Delegate* delegate) = 0;
    virtual void quit() = 0;
    virtual void schedule_work() = 0;
    virtual void schedule_delayed_work(const Delegate::NextWorkInfo& next_work_info) = 0;
};

}  // namespace base
