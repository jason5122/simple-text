#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace scroll_trace {

inline constexpr char kHeader[] = "# px-scroll-trace-v1";

struct Sample {
    uint64_t time_ns = 0;
    double delta_x = 0.0;
    double delta_y = 0.0;
    double scrolling_delta_x = 0.0;
    double scrolling_delta_y = 0.0;
    bool precise = false;
    uint64_t phase = 0;
    uint64_t momentum_phase = 0;
    int64_t line_delta_x = 0;
    int64_t line_delta_y = 0;
    double fixed_delta_x = 0.0;
    double fixed_delta_y = 0.0;
    int64_t point_delta_x = 0;
    int64_t point_delta_y = 0;
    bool continuous = false;
};

bool read_trace(const char* path, std::vector<Sample>* samples, std::string* error);

}  // namespace scroll_trace
