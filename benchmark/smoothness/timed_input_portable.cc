#include "benchmark/smoothness/timed_input.h"
#include "px/px.h"
#include <cmath>

void schedule_timed_input(const std::vector<uint64_t>& timestamps_ns,
                          uint64_t phase_ns,
                          std::function<void(size_t, double)> callback) {
    const double start = px_now();
    for (size_t index = 0; index < timestamps_ns.size(); ++index) {
        const double delay_ms = static_cast<double>(timestamps_ns[index] + phase_ns) / 1'000'000.0;
        const double scheduled = start + delay_ms / 1000.0;
        px_set_timeout([callback, index, scheduled] { callback(index, scheduled); },
                       static_cast<int>(std::llround(delay_ms)));
    }
}
