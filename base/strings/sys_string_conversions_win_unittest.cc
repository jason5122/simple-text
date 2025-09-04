#include "base/strings/sys_string_conversions.h"
#include <gtest/gtest.h>

namespace base {

namespace {
static const std::wstring kSysWideOldItalicLetterA = L"\xd800\xdf00";
}

TEST(SysStrings, SysWideToUTF8) {
    EXPECT_EQ("Hello, world", sys_wide_to_utf8(L"Hello, world"));
    EXPECT_EQ("\xe4\xbd\xa0\xe5\xa5\xbd", sys_wide_to_utf8(L"\x4f60\x597d"));

    // A value outside of the BMP and therefore not representable with one UTF-16
    // code unit.
    EXPECT_EQ("\xF0\x90\x8C\x80", sys_wide_to_utf8(kSysWideOldItalicLetterA));

    // Error case. When Windows finds a UTF-16 character going off the end of
    // a string, it just converts that literal value to UTF-8, even though this
    // is invalid.
    //
    // This is what XP does, but Vista has different behavior, so we don't bother
    // verifying it:
    // EXPECT_EQ("\xE4\xBD\xA0\xED\xA0\x80zyxw",
    //           sys_wide_to_utf8(L"\x4f60\xd800zyxw"));

    // Test embedded NULLs.
    std::wstring wide_null(L"a");
    wide_null.push_back(0);
    wide_null.push_back('b');

    std::string expected_null("a");
    expected_null.push_back(0);
    expected_null.push_back('b');

    EXPECT_EQ(expected_null, sys_wide_to_utf8(wide_null));
}

TEST(SysStrings, SysUTF8ToWide) {
    EXPECT_EQ(L"Hello, world", sys_utf8_to_wide("Hello, world"));
    EXPECT_EQ(L"\x4f60\x597d", sys_utf8_to_wide("\xe4\xbd\xa0\xe5\xa5\xbd"));

    // A value outside of the BMP and therefore not representable with one UTF-16
    // code unit.
    EXPECT_EQ(kSysWideOldItalicLetterA, sys_utf8_to_wide("\xF0\x90\x8C\x80"));

    // Error case. When Windows finds an invalid UTF-8 character, it just skips
    // it. This seems weird because it's inconsistent with the reverse conversion.
    //
    // This is what XP does, but Vista has different behavior, so we don't bother
    // verifying it:
    // EXPECT_EQ(L"\x4f60zyxw", sys_utf8_to_wide("\xe4\xbd\xa0\xe5\xa5zyxw"));

    // Test embedded NULLs.
    std::string utf8_null("a");
    utf8_null.push_back(0);
    utf8_null.push_back('b');

    std::wstring expected_null(L"a");
    expected_null.push_back(0);
    expected_null.push_back('b');

    EXPECT_EQ(expected_null, sys_utf8_to_wide(utf8_null));
}

}  // namespace base
