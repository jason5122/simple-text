#include "unicode/unicode.h"
#include <gtest/gtest.h>

namespace unicode {

TEST(UnicodeTest, CountUTF8Test) {
    std::string str1 = "hello world";
    EXPECT_EQ(count_utf8(str1.data(), str1.length()), 11);

    std::string str2 = "⌚..⌛⏩..⏬☂️..☃️";
    EXPECT_EQ(count_utf8(str2.data(), str2.length()), 14);

    std::string str3 = "﷽";
    EXPECT_EQ(count_utf8(str3.data(), str3.length()), 1);
}

}  // namespace unicode
