#include "base/debug/timer.h"
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace {

constexpr auto operator*(const std::string_view& sv, size_t times) {
    std::string result;
    for (size_t i = 0; i < times; ++i) result += sv;
    return result;
}

auto benchmark(size_t exp) {
    base::Timer timer;

    size_t size = std::pow(10, exp);
    std::string str = std::string_view("a") * size;
    std::string line;
    for (char c : str) {
        if (c == '\n') {
            // Do something with line.
            line.clear();
        } else {
            line += c;
        }
    }

    auto duration = timer.stop();
    spdlog::info("benchmark(10^{}): {} µs", exp, duration);
    return duration;
}

}  // namespace

int main() {
    for (size_t exp = 3; exp <= 9; exp++) {
        benchmark(exp);
    }
}
