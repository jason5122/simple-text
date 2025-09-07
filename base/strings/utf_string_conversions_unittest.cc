#include "utf_string_conversions.h"
#include <gtest/gtest.h>

namespace base {

namespace {
constexpr auto kConvertRoundtripCases = std::to_array<std::string_view>({
    "",               // Empty string
    "Hello, World!",  // ASCII / ISO-8859 string (also valid UTF-8)
    "a\0b",           // UTF-8 with embedded NUL byte
    "λf",             // lowercase lambda + 'f'
    "χρώμιο",         // "chromium" in greek
    "כרום",           // "chromium" in hebrew
    "クロム",         // "chromium" in japanese
});
}  // namespace

TEST(UTFStringConversionsTest, RoundTripsFromUTF8) {
    for (const auto& string8 : kConvertRoundtripCases) {
        std::u16string string16 = utf8_to_utf16(string8);
        std::string back8 = utf16_to_utf8(string16);
        EXPECT_EQ(string8, back8);
    }
}

TEST(UTFStringConversionsTest, RoundTripsFromUTF16) {
    for (const auto& original : kConvertRoundtripCases) {
        std::u16string string16 = utf8_to_utf16(original);
        std::string string8 = utf16_to_utf8(string16);
        std::u16string back16 = utf8_to_utf16(string8);
        EXPECT_EQ(string16, back16);
    }
}

}  // namespace base
