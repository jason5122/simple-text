#pragma once

#include "util/non_copyable.h"
#include "util/std_print.h"
#include <chrono>

// https://stackoverflow.com/a/37607676
template <typename Duration = std::chrono::microseconds>
class Profiler {
public:
    Profiler(std::string const& n) : name(n), t1(std::chrono::high_resolution_clock::now()) {}
    ~Profiler() {
        // auto t2 = std::chrono::high_resolution_clock::now();
        // auto duration = std::chrono::duration_cast<Duration>(t2 - t1).count();

        // std::string unit = "units";
        // if (std::is_same<Duration, std::chrono::microseconds>::value) {
        //     unit = "µs";
        // } else if (std::is_same<Duration, std::chrono::milliseconds>::value) {
        //     unit = "ms";
        // }
        // std::println("{}: {} {}", name, duration, unit);
    }

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point t1;
};

#define PROFILE_BLOCK(name) const Profiler _pfinstance(name)
#define PROFILE_BLOCK_WITH_DURATION(name, duration) const Profiler<duration> _pfinstance(name)

// template <typename Duration = std::chrono::microseconds>
// inline Profiler<Duration> ProfileBlock(std::string name) {
//     return Profiler<Duration>(name);
// }
