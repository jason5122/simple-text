#pragma once

#include <chrono>
#include <iostream>
#include <string>

namespace base {

class Timer {
public:
    Timer() : t1(std::chrono::high_resolution_clock::now()) {}

    auto stop() {
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        return duration;
    }

private:
    std::chrono::high_resolution_clock::time_point t1;
};

}  // namespace base
