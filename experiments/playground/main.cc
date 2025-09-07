#include "unicode/unicode.h"
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("{:?}", "hello world!\n\n\n\r\n\t");
    spdlog::info(unicode::utf8_to_utf16_length("Hello😄🙂hi"));
}
