#include "base/unicode.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace base {

namespace {

constexpr auto kInvalids = std::to_array<std::string_view>({
    "\x80",              // lone continuation
    "\xE2\x28\xA1",      // wrong continuation
    "\xC2",              // truncated 2-byte
    "\xE2\x82",          // truncated 3-byte
    "\xF0\x9F\x92",      // truncated 4-byte
    "\xC0\x80",          // overlong via illegal lead
    "\xF5\x80\x80\x80",  // > U+10FFFF
    "\xFF",              // illegal byte
});

// (byte offset, code point) per code point, in text order.
using Decoded = std::vector<std::pair<size_t, char32_t>>;

Decoded decode_forward(std::string_view utf8) {
    Decoded decoded;
    for (size_t i = 0; i < utf8.size();) {
        size_t start = i;
        char32_t cp = decode_utf8(utf8, i);
        decoded.emplace_back(start, cp);
    }
    return decoded;
}

Decoded decode_backward(std::string_view utf8) {
    Decoded decoded;
    for (size_t i = utf8.size(); i > 0;) {
        char32_t cp = decode_prev_utf8(utf8, i);
        decoded.emplace_back(i, cp);
    }
    std::ranges::reverse(decoded);
    return decoded;
}

}  // namespace

TEST(UTF8SequenceLengthTest, CountsBytesFromLeadByte) {
    EXPECT_EQ(utf8_sequence_length('a'), 1UZ);
    EXPECT_EQ(utf8_sequence_length('\xC3'), 2UZ);
    EXPECT_EQ(utf8_sequence_length('\xE2'), 3UZ);
    EXPECT_EQ(utf8_sequence_length('\xF0'), 4UZ);
}

TEST(UTF8SequenceLengthTest, OneForBytesThatCannotBeginASequence) {
    // `decode_utf8` consumes exactly one byte for each of these.
    EXPECT_EQ(utf8_sequence_length('\x80'), 1UZ);  // trail byte
    EXPECT_EQ(utf8_sequence_length('\xC0'), 1UZ);  // overlong lead
    EXPECT_EQ(utf8_sequence_length('\xF5'), 1UZ);  // > U+10FFFF
    EXPECT_EQ(utf8_sequence_length('\xFF'), 1UZ);
}

TEST(DecodePrevUTF8Test, DecodesValidText) {
    // "Foo" | © (2 bytes) | "bar" | 𝌆 (4 bytes) | "baz" | ☃ (3 bytes) | "qux"
    EXPECT_EQ(decode_backward("Foo©bar𝌆baz☃qux"), (Decoded{{0, U'F'},
                                                           {1, U'o'},
                                                           {2, U'o'},
                                                           {3, U'\u00A9'},
                                                           {5, U'b'},
                                                           {6, U'a'},
                                                           {7, U'r'},
                                                           {8, U'\U0001D306'},
                                                           {12, U'b'},
                                                           {13, U'a'},
                                                           {14, U'z'},
                                                           {15, U'\u2603'},
                                                           {18, U'q'},
                                                           {19, U'u'},
                                                           {20, U'x'}}));
}

TEST(DecodePrevUTF8Test, ReplacesIllFormedSequences) {
    // A truncated sequence is one replacement character; each stray trail byte is its own.
    EXPECT_EQ(decode_backward("\xE2\x82"), (Decoded{{0, U'\uFFFD'}}));
    EXPECT_EQ(decode_backward("\x80\x80"), (Decoded{{0, U'\uFFFD'}, {1, U'\uFFFD'}}));
    EXPECT_EQ(decode_backward("a\xE9"), (Decoded{{0, U'a'}, {1, U'\uFFFD'}}));
}

TEST(DecodePrevUTF8Test, AgreesWithForwardDecodeOnIllFormedInput) {
    for (std::string_view invalid : kInvalids) {
        for (const std::string& s :
             {std::string{invalid}, "a" + std::string{invalid} + "b",
              "é" + std::string{invalid} + "☃", std::string{invalid} + std::string{invalid}}) {
            EXPECT_EQ(decode_backward(s), decode_forward(s)) << testing::PrintToString(s);
        }
    }
}

TEST(UTF16ToUTF8OffsetsTest, MapsEachUTF16UnitToItsCodePointStart) {
    // "Foo" | © (2 bytes) | "bar" | 𝌆 (4 bytes, surrogate pair) | "baz" | ☃ (3 bytes) | "qux"
    auto offsets = utf16_to_utf8_offsets("Foo©bar𝌆baz☃qux");
    EXPECT_EQ(offsets,
              (std::vector<size_t>{0, 1, 2, 3, 5, 6, 7, 8, 8, 12, 13, 14, 15, 18, 19, 20}));
}

TEST(UTF16ToUTF8OffsetsTest, SurrogatePairSharesOneOffset) {
    // clang-format off
    auto offsets = utf16_to_utf8_offsets("A" "\xF0\x9F\x98\x80" "B");
    // clang-format on
    EXPECT_EQ(offsets, (std::vector<size_t>{0, 1, 1, 5}));
}

TEST(UTF16ToUTF8OffsetsTest, EmptyForEmptyInput) {
    auto offsets = utf16_to_utf8_offsets("");
    EXPECT_TRUE(offsets.empty());
}

}  // namespace base
