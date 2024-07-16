#include "base/buffer/buffer.h"
#include "gtest/gtest.h"
#include <string>

#include <format>
#include <iostream>

TEST(BufferTest, Insert) {
    static const std::string kExample1 = R"(Hello world!
hi there)";

    base::Buffer buffer{kExample1};

    std::cerr << std::format("Before: \"{}\"\n", buffer.str());
    for (size_t i = 0; i < buffer.lineCount(); i++) {
        std::string_view line = buffer.getLineContents(i);
        std::cerr << line << '\n';
    }

    buffer.insert(buffer.stringEnd(), "\nthis is a newline");
    std::cerr << std::format("After: \"{}\"\n", buffer.str());
    for (size_t i = 0; i < buffer.lineCount(); i++) {
        std::string_view line = buffer.getLineContents(i);
        std::cerr << line << '\n';
    }
}
