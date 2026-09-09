#include "benchmark/smoothness/timed_input.h"
#include "px/px.h"
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#include <thread>

namespace {

uint64_t nanoseconds_to_ticks(uint64_t nanoseconds, const mach_timebase_info_data_t& timebase) {
    const unsigned __int128 scaled = static_cast<unsigned __int128>(nanoseconds) * timebase.denom;
    return static_cast<uint64_t>(scaled / timebase.numer);
}

}  // namespace

void schedule_timed_input(const std::vector<uint64_t>& timestamps_ns,
                          uint64_t phase_ns,
                          std::function<void(size_t, double)> callback) {
    std::thread([timestamps_ns, phase_ns, callback = std::move(callback)] {
        mach_timebase_info_data_t timebase{};
        mach_timebase_info(&timebase);
        const double start_seconds = px_now() + static_cast<double>(phase_ns) / 1e9;
        const uint64_t start = mach_absolute_time() + nanoseconds_to_ticks(phase_ns, timebase);
        for (size_t index = 0; index < timestamps_ns.size(); ++index) {
            mach_wait_until(start + nanoseconds_to_ticks(timestamps_ns[index], timebase));
            const double scheduled =
                start_seconds + static_cast<double>(timestamps_ns[index]) / 1e9;
            dispatch_async(dispatch_get_main_queue(), ^{
              callback(index, scheduled);
            });
        }
    }).detach();
}
