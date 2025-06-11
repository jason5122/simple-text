#include "base/buffer/aho_corasick/aho_corasick.h"
#include "util/random.h"
#include <gtest/gtest.h>

namespace base {

using MatchResult = AhoCorasick::MatchResult;

namespace {

MatchResult MatchPattern(const PieceTree& tree, std::string_view pattern) {
    AhoCorasick ac({std::string(pattern)});
    auto result = ac.match(tree);
    return result;
}

MatchResult MatchDict(const PieceTree& tree, const std::vector<std::string>& dict) {
    AhoCorasick ac(dict);
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

void CheckRandom(std::string_view str) {
    size_t i = util::random_number(0, str.length() - 1);
    size_t len = util::random_number(1, str.length());
    std::string_view pattern = str.substr(i, len);

    PieceTree tree{str};
    auto result = MatchPattern(tree, pattern);
    CheckResult(result, str, pattern);
}
}  // namespace

TEST(AhoCorasickTest, RandomTest) {
    std::string str = "hello world!";
    for (int i = 0; i < 100; ++i) {
        CheckRandom(str);
    }
}

TEST(AhoCorasickTest, RandomCharTest) {
    // This string contains chars of *any* value. This is not necessarily valid Unicode.
    auto random_char_str = []() {
        std::string str;
        for (int j = 0; j < 100; ++j) str += util::random_char();
        return str;
    };

    for (int i = 0; i < 100; ++i) {
        CheckRandom(random_char_str());
    }
}

namespace {
struct StrPair {
    std::string str;
    std::optional<std::string> match;
};

using Dict = std::vector<std::string>;
using StrPairs = std::vector<StrPair>;

void TestCase(const StrPairs& str_pairs, const Dict& dict) {
    for (const auto& [str, expected] : str_pairs) {
        PieceTree tree{str};
        auto result = MatchDict(tree, dict);
        CheckResult(result, str, expected);
    }
}
}  // namespace

TEST(AhoCorasickTest, Test1) {
    Dict dict = {"he", "she", "his", "her"};
    StrPairs str_pairs = {{"he", "he"},  {"she", "she"}, {"his", "his"},   {"hers", "he"},
                          {"ahe", "he"}, {"shhe", "he"}, {"shis2", "his"}, {"ahhe", "he"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test2) {
    Dict dict = {"poto", "poto"};
    StrPairs str_pairs = {{"The pot had a handle", std::nullopt}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test3) {
    Dict dict = {"The"};
    StrPairs str_pairs = {{"The pot had a handle", "The"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test4) {
    Dict dict = {"pot"};
    StrPairs str_pairs = {{"The pot had a handle", "pot"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test5) {
    Dict dict = {"pot "};
    StrPairs str_pairs = {{"The pot had a handle", "pot "}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test6) {
    Dict dict = {"ot h"};
    StrPairs str_pairs = {{"The pot had a handle", "ot h"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test7) {
    Dict dict = {"andle"};
    StrPairs str_pairs = {{"The pot had a handle", "andle"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test8) {
    Dict dict = {"aaab"};
    StrPairs str_pairs = {{"aaaaaaab", "aaab"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test9) {
    Dict dict = {"haha", "z"};
    StrPairs str_pairs = {{"aaaaz", "z"}, {"z", "z"}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test10) {
    Dict dict = {"abc"};
    StrPairs str_pairs = {{"cde", std::nullopt}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test11) {
    Dict dict = {"‼️", "ØØ"};
    StrPairs str_pairs = {
        {"hello‼️", "‼️"},
        {"asdf", std::nullopt},
        {"ØØØ", "ØØ"},
        {"Ø", std::nullopt},
    };
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test12) {
    Dict dict = {"abc﷽def"};
    StrPairs str_pairs = {
        {"hello﷽worldabc﷽abc﷽def", "abc﷽def"},
        {"abc\x{EF}\x{B7}\x{BD}def", "abc﷽def"},
        {"abc\x{EF}\x{B7}\x{B7}\x{BD}def", std::nullopt},
        {"abc\x{EF}\x{B7}\x{BD}def\x{BD}", "abc﷽def"},
    };
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test13) {
    Dict dict = {""};
    StrPairs str_pairs = {{"", std::nullopt}, {"abc", std::nullopt}};
    TestCase(str_pairs, dict);
}

TEST(AhoCorasickTest, Test14) {
    Dict dict = {"3"};
    StrPairs str_pairs = {{"abc123", "3"}, {"3", "3"}};
    TestCase(str_pairs, dict);
}

}  // namespace base
