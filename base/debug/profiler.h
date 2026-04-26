#pragma once

#include <chrono>
#include <print>
#include <string>

namespace base {

class Profiler {
public:
    Profiler(std::string_view name) : name(name), t1(std::chrono::high_resolution_clock::now()) {}
    ~Profiler() {
        if (stopped) return;
        stop_micro();
    }

    void stop_micro() {
        if (stopped) {
            std::println("Warning: profiler was already stopped.");
            return;
        }
        stopped = true;
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        std::println("{}: {} µs", name, duration);
    }

    void stop_mili() {
        if (stopped) {
            std::println("Warning: profiler was already stopped.");
            return;
        }
        stopped = true;
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        std::println("{}: {} ms", name, duration);
    }

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point t1;
    bool stopped = false;
};

}  // namespace base
