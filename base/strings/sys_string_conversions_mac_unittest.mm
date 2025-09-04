#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include <Foundation/Foundation.h>
#include <gtest/gtest.h>
#include <vector>

namespace base {

TEST(SysStrings, ConversionsFromNSString) {
    EXPECT_STREQ("Hello, world!", sys_nsstring_to_utf8(@"Hello, world!").c_str());

    // Conversions should be able to handle a NULL value without crashing.
    EXPECT_STREQ("", sys_nsstring_to_utf8(nil).c_str());
    EXPECT_EQ(std::u16string(), sys_nsstring_to_utf16(nil));
}

namespace {
std::vector<std::string> kConvertRoundtripCases({
    "Hello, World!",  // ASCII / ISO-8859 string (also valid UTF-8)
    "a\0b",           // UTF-8 with embedded NUL byte
    "λf",             // lowercase lambda + 'f'
    "χρώμιο",         // "chromium" in greek
    "כרום",           // "chromium" in hebrew
    "クロム",         // "chromium" in japanese

    // Tarot card symbol "the morning", which is outside of the BMP and is not
    // representable with one UTF-16 code unit.
    "🃦",
});
}  // namespace

TEST(SysStrings, RoundTripsFromUTF8) {
    for (const auto& string8 : kConvertRoundtripCases) {
        NSString* nsstring8 = sys_utf8_to_nsstring(string8);
        std::string back8 = sys_nsstring_to_utf8(nsstring8);
        EXPECT_EQ(string8, back8);
    }
}

TEST(SysStrings, RoundTripsFromUTF16) {
    for (const auto& string8 : kConvertRoundtripCases) {
        std::u16string string16 = utf8_to_utf16(string8);
        NSString* nsstring16 = sys_utf16_to_nsstring(string16);
        std::u16string back16 = sys_nsstring_to_utf16(nsstring16);
        EXPECT_EQ(string16, back16);
    }
}

}  // namespace base
