#include "base/buffer/aho_corasick/aho_corasick.h"
#include "util/profiler.h"
#include <gtest/gtest.h>

namespace base {

using MatchResult = AhoCorasick::MatchResult;

namespace {

MatchResult MatchPattern(const PieceTree& tree, std::string_view pattern) {
    AhoCorasick ac({std::string(pattern)});
    auto result = ac.match(tree);
    return result;
}

void CheckResult(const MatchResult& r,
                 std::string_view str,
                 const std::optional<std::string_view>& expected) {
    int begin = r.match_begin;
    int end = r.match_end;

    // Check if the return value is sane.
    EXPECT_TRUE(begin <= end);

    // The string is not supposed to match the pattern.
    if (!expected) {
        ASSERT_TRUE(begin == -1 && end == -1);
    }
    // The string matches the pattern.
    else {
        ASSERT_TRUE(begin >= 0 && end >= 0);

        size_t len = end - begin + 1;
        ASSERT_EQ(len, (*expected).length());

        std::string_view substr = str.substr(begin, len);
        EXPECT_EQ(substr, expected);

        size_t pos = str.find(*expected);
        EXPECT_EQ(static_cast<size_t>(begin), pos);
    }
}

constexpr auto operator*(const std::string_view& sv, size_t times) {
    std::string result;
    for (size_t i = 0; i < times; ++i) {
        result += sv;
    }
    return result;
}

// TODO: Speed up creation of the long string and piece tree.
const std::string kX = "x";
const std::string kLongLine = kX * 100;
const std::string kStr1Gb = kLongLine * 10000000;
const std::string kLongStr = "needle" + kStr1Gb;
const PieceTree kLongPieceTree{kLongStr};

}  // namespace

// Test that matching quits as soon as possible. We don't want to load the entire string if we
// don't have to. These tests should return immediately.
// TODO: Make this optional. This test runs slowly.
TEST(AhoCorasickPerfTest, LongStringTest) {
    auto result = MatchPattern(kLongPieceTree, "needle");
    CheckResult(result, kLongStr, "needle");
}

/*
Aho-Corasick match (string): 7898 ms
Aho-Corasick match (piece table): 17706 ms
std::string find: 50 ms
std::string iteration: 11498 ms
Piece table iteration: 14623 ms
__builtin_char_memchr: 25 ms
*/

// TODO: Make this optional. This test runs slowly; only run if you need to re-measure performance.
TEST(AhoCorasickPerfTest, VariousIterationTests) {
    auto pf1 = util::Profiler{"PieceTree line-by-line iteration"};
    for (size_t line = 0; line < kLongPieceTree.line_count(); ++line) {
        std::string line_str = kLongPieceTree.get_line_content(line);
    }
    pf1.stop_mili();

    auto ac = AhoCorasick({"a"});

    auto pf2 = util::Profiler{"Aho-Corasick match"};
    ac.match(kLongPieceTree);
    pf2.stop_mili();

    auto pf3 = util::Profiler{"std::string find"};
    static_cast<void>(kLongStr.find("a"));
    pf3.stop_mili();

    auto pf4 = util::Profiler{"std::string iteration"};
    for (char ch [[maybe_unused]] : kLongStr) {
    }
    pf4.stop_mili();

    auto pf5 = util::Profiler{"Piece table iteration"};
    TreeWalker walker{&kLongPieceTree};
    while (!walker.exhausted()) {
        walker.next();
    }
    pf5.stop_mili();

    auto pf6 = util::Profiler{"__builtin_char_memchr"};
    __builtin_char_memchr(kLongStr.data(), 'a', kLongStr.size());
    pf6.stop_mili();
}

}  // namespace base
