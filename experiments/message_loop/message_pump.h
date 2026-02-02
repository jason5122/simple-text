#pragma once

#include "base/time/time.h"
#include "build/build_config.h"
#include <memory>

#if BUILDFLAG(IS_WIN)
constexpr TimeDelta kDefaultLeeway = base::TimeDelta::milliseconds(16);
#else
constexpr base::TimeDelta kDefaultLeeway = base::TimeDelta::milliseconds(8);
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

            base::TimeDelta remaining_delay() const {
                return delayed_run_time - base::TimeTicks::now();
            }

            // The next PendingTask's |delayed_run_time|. is_null() if there's extra work to run
            // immediately. is_max() if there are no more immediate nor delayed tasks.
            base::TimeTicks delayed_run_time;

            // |leeway| determines the preferred time range for scheduling work. A larger leeway
            // provides more freedom to schedule work at
            // an optimal time for power consumption. This field is ignored
            // for immediate work.
            base::TimeDelta leeway = kDefaultLeeway;
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
