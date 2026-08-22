#include "experiments/rope/rope.h"

#include <algorithm>
#include <format>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace rope {
namespace {

static_assert(std::movable<Rope>);

// Mirrors Rope::fake_char_width() (rope.cc) -- duplicated rather than
// exposed, since it's an internal placeholder for a real font backend, not
// part of the public API.
double reference_char_width(char c) {
    switch (c) {
    case '\t':
        return 4.0;
    case 'i':
    case 'l':
    case '.':
    case ',':
    case '\'':
        return 0.5;
    case 'm':
    case 'w':
    case 'M':
    case 'W':
        return 1.5;
    default:
        return 1.0;
    }
}

// Reference "widest line" computation: split on '\n', measure each piece
// with reference_char_width(), take the max.
double widest_line_width_of(const std::string& s) {
    double widest = 0;
    size_t start = 0;
    while (true) {
        size_t end = s.find('\n', start);
        std::string_view piece = end == std::string::npos
                                     ? std::string_view(s).substr(start)
                                     : std::string_view(s).substr(start, end - start);
        double width = 0;
        for (char c : piece) {
            width += reference_char_width(c);
        }
        widest = std::max(widest, width);
        if (end == std::string::npos) {
            return widest;
        }
        start = end + 1;
    }
}

TEST(RopeTest, DefaultConstructedIsEmpty) {
    Rope r;
    EXPECT_EQ(r.str(), "");
}

TEST(RopeTest, ConstructFromShortString) {
    Rope r("hi");  // Shorter than one leaf.
    EXPECT_EQ(r.str(), "hi");
}

TEST(RopeTest, ConstructForcesMultipleSplits) {
    // Long enough that building it in one insert() call overflows a leaf
    // many times over, then overflows the internal node that leaf-splitting
    // produces -- the exact shape that exposed a real ordering bug in
    // split_overflowing_internal (it collected multi-way overflow siblings
    // out of left-to-right order).
    std::string text;
    for (int i = 0; i < 2500; ++i) {
        text += "The quick brown fox jumps over the lazy dog. ";
    }
    Rope r(text);
    EXPECT_EQ(r.str(), text);
}

TEST(RopeTest, InsertAtFrontMiddleAndEnd) {
    Rope r("The quick brown fox jumps over the lazy dog");

    r.insert(0, "[[");
    r.insert(r.size(), "]]");
    r.insert(r.size() / 2, " ***MIDDLE*** ");

    EXPECT_EQ(r.str(), "[[The quick brown fox j ***MIDDLE*** umps over the lazy dog]]");
}

TEST(RopeTest, InsertEmptyStringIsNoOp) {
    Rope r("hello");
    r.insert(2, "");
    EXPECT_EQ(r.str(), "hello");
}

TEST(RopeTest, InsertCausesMultiWayInternalOverflow) {
    // One insert() call producing far more new leaves than a single split
    // can absorb, forcing split_overflowing_internal to run several
    // iterations -- the same shape as the bug referenced above.
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);
    EXPECT_EQ(r.str(), text);
}

TEST(RopeTest, EraseFromFrontUntilEmpty) {
    Rope r("abc");
    r.erase(0, 1);
    EXPECT_EQ(r.str(), "bc");
    r.erase(0, 1);
    EXPECT_EQ(r.str(), "c");
    r.erase(0, 1);
    EXPECT_EQ(r.str(), "");
}

TEST(RopeTest, EraseEmptiesALeafAndRelinksChain) {
    // Two full leaves' worth of text (rope.h documents the leaf cap), as
    // two distinct runs so it's obvious where the leaf boundary was.
    // Erasing the first leaf entirely must repoint the second leaf to the
    // front of the chain rather than leaving str() walk into a freed leaf.
    std::string text(4096, 'A');
    text += std::string(4096, 'B');
    Rope r(text);

    r.erase(0, 4096);
    EXPECT_EQ(r.str(), std::string(4096, 'B'));
}

TEST(RopeTest, EraseSpanningMultipleLeavesAndInternalChildren) {
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);

    text.erase(5, 80000);
    r.erase(5, 80000);
    EXPECT_EQ(r.str(), text);
}

TEST(RopeTest, EraseEverythingEmptiesTheRope) {
    Rope r("hello world");
    r.erase(0, r.size());
    EXPECT_EQ(r.str(), "");
}

TEST(RopeTest, EraseZeroCountIsNoOp) {
    Rope r("hello");
    r.erase(2, 0);
    EXPECT_EQ(r.str(), "hello");
}

TEST(RopeTest, CharAtMatchesEveryPosition) {
    // char_at() descends the tree by cached length; str() just walks the
    // linked leaf chain. The two share almost no code, so this is checked
    // once here rather than folded into every test above.
    std::string text = "The quick brown fox jumps over the lazy dog";
    Rope r(text);
    for (size_t i = 0; i < text.size(); ++i) {
        EXPECT_EQ(r.char_at(i), text[i]) << "at position " << i;
    }
}

TEST(RopeTest, LineCountAndLineAt) {
    std::string text = "Line one\nLine two is longer\nShort\n\nLine five\nLast line";
    Rope r(text);
    EXPECT_EQ(r.line_count(), size_t{6});

    size_t line = 0;
    for (size_t i = 0; i <= text.size(); ++i) {
        EXPECT_EQ(r.line_at(i), line) << "at position " << i;
        if (i < text.size() && text[i] == '\n') {
            ++line;
        }
    }
}

TEST(RopeTest, LineContent) {
    Rope r("hello\nworld\nlast line");
    EXPECT_EQ(r.line_content(0), "hello");
    EXPECT_EQ(r.line_content(1), "world");
    EXPECT_EQ(r.line_content(2), "last line");
}

TEST(RopeTest, LineContentWithEmptyLines) {
    Rope r("a\n\nb");
    EXPECT_EQ(r.line_content(0), "a");
    EXPECT_EQ(r.line_content(1), "");
    EXPECT_EQ(r.line_content(2), "b");
}

TEST(RopeTest, WidestLineWidth) {
    // "hi" = 1 + 0.5 = 1.5; "www" = 1.5 * 3 = 4.5; "x" = 1.
    Rope r("hi\nwww\nx");
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 4.5);
}

TEST(RopeTest, WidestLineWidthPrefersWidthOverCharacterCount) {
    // "wwww" is 4 characters but width 6; "iiiiiiii" is 8 characters (twice
    // as many) but width 4. If this measured length instead of width, it'd
    // pick the wrong line.
    Rope r("wwww\niiiiiiii");
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 6.0);
}

TEST(RopeTest, WidestLineWidthSpansMultipleLeaves) {
    // A single line longer than one leaf (rope.h documents a 4096-char
    // cap), so computing its width requires LineWidthMetric::combine to
    // handle a line crossing a leaf boundary, not just per-leaf maxima.
    std::string wide_line(5000, 'a');  // 5000 * fake_char_width('a') == 5000.
    Rope r("short\n" + wide_line + "\nalso short");
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 5000.0);
}

TEST(RopeTest, WidestLineWidthOfEmptyRopeIsZero) {
    Rope r;
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 0.0);
}

// ---- Property-based (FuzzTest) differential tests against std::string ----
// These hold a plain std::string as the reference model, apply the same
// operations to it and to the Rope, and assert the two stay in sync. The
// fuzzer mutates toward interesting inputs (empty strings, embedded '\n',
// boundary offsets), covering far more states than the fixed cases above.
// Run one continuously with e.g. `unit_tests --fuzz=RopeFuzzTest.EditsMatchStringModel`.

void ConstructionMatchesInput(const std::string& s) {
    Rope r(s);
    EXPECT_EQ(r.str(), s);
}
FUZZ_TEST(RopeFuzzTest, ConstructionMatchesInput);

void CharAtMatchesString(const std::string& s) {
    Rope r(s);
    for (size_t i = 0; i < s.size(); ++i) {
        ASSERT_EQ(r.char_at(i), s[i]);
    }
}
FUZZ_TEST(RopeFuzzTest, CharAtMatchesString);

// An arbitrary sequence of inserts and erases keeps the rope equal to the
// same edits applied to a std::string. Each edit is (is_insert, offset,
// count, text); offsets/counts are taken modulo the current length so
// they're always valid for both.
void EditsMatchStringModel(
    const std::string& initial,
    const std::vector<std::tuple<bool, size_t, size_t, std::string>>& edits) {
    std::string str = initial;
    Rope r(initial);

    for (const auto& [is_insert, raw_pos, raw_count, text] : edits) {
        if (is_insert) {
            size_t pos = raw_pos % (str.size() + 1);
            str.insert(pos, text);
            r.insert(pos, text);
        } else {
            if (str.empty()) {
                continue;
            }
            size_t pos = raw_pos % str.size();
            size_t count = raw_count % (str.size() - pos + 1);
            str.erase(pos, count);
            r.erase(pos, count);
        }
        ASSERT_EQ(r.str(), str);
        // Exercises insert_into_leaf's/erase_from's incremental line_width
        // updates specifically (WidestLineWidthMatchesManualSplit below
        // only ever constructs a rope in one shot, which never touches
        // those incremental paths).
        ASSERT_DOUBLE_EQ(r.widest_line_width(), widest_line_width_of(str));
    }
}
FUZZ_TEST(RopeFuzzTest, EditsMatchStringModel);

// line_at agrees with a direct scan of the text at every position, and
// line_count matches the total number of '\n' bytes plus one.
void LineAtMatchesScan(const std::string& s) {
    Rope r(s);
    size_t line = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        ASSERT_EQ(r.line_at(i), line);
        if (i < s.size() && s[i] == '\n') {
            ++line;
        }
    }
    EXPECT_EQ(r.line_count(), line + 1);
}
FUZZ_TEST(RopeFuzzTest, LineAtMatchesScan);

// line_content agrees with manually splitting the text on '\n'.
void LineContentMatchesManualSplit(const std::string& s) {
    Rope r(s);
    size_t start = 0;
    for (size_t line = 0; line < r.line_count(); ++line) {
        size_t end = s.find('\n', start);
        std::string expected =
            end == std::string::npos ? s.substr(start) : s.substr(start, end - start);
        ASSERT_EQ(r.line_content(line), expected);
        start = end == std::string::npos ? s.size() : end + 1;
    }
}
FUZZ_TEST(RopeFuzzTest, LineContentMatchesManualSplit);

// widest_line_width agrees with manually splitting the text on '\n' and
// measuring each line with the same (fake) per-character width function.
void WidestLineWidthMatchesManualSplit(const std::string& s) {
    Rope r(s);
    ASSERT_DOUBLE_EQ(r.widest_line_width(), widest_line_width_of(s));
}
FUZZ_TEST(RopeFuzzTest, WidestLineWidthMatchesManualSplit);

}  // namespace
}  // namespace rope
