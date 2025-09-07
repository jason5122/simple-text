#include "base/rand_util.h"
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("{:?}", "hello world!\n\n\n\r\n\t");

    std::vector<uint8_t> data(10);
    base::rand_bytes(data);
    spdlog::info(data);
}
