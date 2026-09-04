#include "experiments/platform/smoothness/scroll_trace.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace scroll_trace {
namespace {

bool parse_sample(const std::string& line, Sample* sample) {
    int precise = 0;
    int continuous = 0;
    std::istringstream input(line);
    if (!(input >> sample->time_ns >> sample->delta_x >> sample->delta_y >>
          sample->scrolling_delta_x >> sample->scrolling_delta_y >> precise >> sample->phase >>
          sample->momentum_phase >> sample->line_delta_x >> sample->line_delta_y >>
          sample->fixed_delta_x >> sample->fixed_delta_y >> sample->point_delta_x >>
          sample->point_delta_y >> continuous)) {
        return false;
    }
    input >> std::ws;
    sample->precise = precise != 0;
    sample->continuous = continuous != 0;
    return input.eof() && std::isfinite(sample->delta_x) && std::isfinite(sample->delta_y) &&
           std::isfinite(sample->scrolling_delta_x) && std::isfinite(sample->scrolling_delta_y) &&
           std::isfinite(sample->fixed_delta_x) && std::isfinite(sample->fixed_delta_y);
}

void set_error(std::string* error, std::string message) {
    if (error) {
        *error = std::move(message);
    }
}

}  // namespace

bool read_trace(const char* path, std::vector<Sample>* samples, std::string* error) {
    if (!path || !samples) {
        set_error(error, "invalid trace destination");
        return false;
    }
    samples->clear();
    if (error) {
        error->clear();
    }

    std::ifstream input(path);
    if (!input) {
        set_error(error, std::string("cannot open ") + path);
        return false;
    }

    std::string line;
    bool found_header = false;
    size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line == kHeader) {
            found_header = true;
            continue;
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }

        Sample sample;
        if (!parse_sample(line, &sample)) {
            set_error(error, "malformed sample at " + std::string(path) + ":" +
                                 std::to_string(line_number));
            samples->clear();
            return false;
        }
        if (!samples->empty() && sample.time_ns < samples->back().time_ns) {
            set_error(error, "timestamps go backwards at " + std::string(path) + ":" +
                                 std::to_string(line_number));
            samples->clear();
            return false;
        }
        samples->push_back(sample);
    }

    if (!found_header) {
        set_error(error, std::string(path) + " is not a version-1 scroll trace");
        samples->clear();
        return false;
    }
    if (samples->empty()) {
        set_error(error, std::string(path) + " contains no samples");
        return false;
    }
    return true;
}

}  // namespace scroll_trace
