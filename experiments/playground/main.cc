#include "base/containers/span_util.h"
#include "base/rand_util.h"
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("{:?}", "hello world!\n\n\n\r\n\t");

    std::vector<uint8_t> data(10);
    base::rand_bytes(data);
    spdlog::info(data);

    spdlog::info("{} {} {} {} {} {}", base::rand_int(1, 6), base::rand_int(1, 6),
                 base::rand_int(1, 6), base::rand_int(1, 6), base::rand_int(1, 6),
                 base::rand_int(1, 6));

    spdlog::info("{:?}", base::rand_bytes_as_string(10));

    uint8_t arr[10] = {};
    base::rand_bytes(arr);

    std::string str(10, ' ');
    base::rand_bytes(base::as_writable_u8_span(str));

    std::string hello = "hello world";
    std::span<char> str_span{hello};
    spdlog::info(str_span);

    uint64_t number = 0x00'11'22'33'44'55'66'77ull;
    spdlog::info(base::as_u8_span(number));
}
