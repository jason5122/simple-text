#include "base/containers/span_util.h"
#include <gtest/gtest.h>

namespace base {

TEST(SpanUtil, StringWritable) {
    constexpr auto kStrings = std::to_array<std::string_view>({"", "hello world", "😀😀😀"});
    for (auto sv : kStrings) {
        std::string s(sv);
        std::span<uint8_t> bytes = as_writable_u8_span(s);
        EXPECT_EQ(bytes.size(), s.size());
        ASSERT_NE(bytes.data(), nullptr);
        EXPECT_EQ(bytes.data(), reinterpret_cast<uint8_t*>(s.data()));
    }
}

TEST(SpanUtil, StringReadable) {
    constexpr auto kStrings = std::to_array<std::string_view>({"", "hello world", "😀😀😀"});
    for (auto sv : kStrings) {
        std::string s(sv);
        std::span<const uint8_t> bytes = as_u8_span(s);
        EXPECT_EQ(bytes.size(), s.size());
        ASSERT_NE(bytes.data(), nullptr);
        EXPECT_EQ(bytes.data(), reinterpret_cast<uint8_t*>(s.data()));
    }
}

TEST(SpanUtil, SingleObjectUint64Writable) {
    uint64_t x = 0;
    std::span<uint8_t> bytes = as_writable_u8_span(x);
    EXPECT_EQ(bytes.size(), sizeof(x));

    if constexpr (std::endian::native == std::endian::little) {
        bytes[0] = 0x11;
    } else {
        bytes[7] = 0x11;
    }
    EXPECT_EQ(x, 0x11);
}

TEST(SpanUtil, SingleObjectUint64Readable) {
    uint64_t x = 0x00'11'22'33'44'55'66'77ull;
    std::span<const uint8_t> bytes = as_u8_span(x);
    EXPECT_EQ(bytes.size(), sizeof(x));

    auto kExpected =
        std::to_array<const uint8_t>({0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77});

    if constexpr (std::endian::native == std::endian::little) {
        std::reverse(kExpected.begin(), kExpected.end());
    }

    EXPECT_TRUE(std::ranges::equal(bytes, kExpected));
}

}  // namespace base
