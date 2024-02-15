#pragma once

#include <chrono>
#include <iostream>

// https://stackoverflow.com/a/37607676
struct profiler {
    std::string name;
    std::chrono::high_resolution_clock::time_point t1;
    profiler(std::string const& n) : name(n), t1(std::chrono::high_resolution_clock::now()) {}
    ~profiler() {
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        std::cerr << name << ": " << duration << " µs\n";
    }
};

#define PROFILE_BLOCK(pbn) profiler _pfinstance(pbn)
