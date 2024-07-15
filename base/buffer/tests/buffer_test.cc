#include "base/buffer/buffer.h"
#include "gtest/gtest.h"
#include <string>

TEST(BufferTest, Insert) {
    static const std::string kExample1 = R"(Hello world!
hi there)";

    base::Buffer buffer{kExample1};
}
