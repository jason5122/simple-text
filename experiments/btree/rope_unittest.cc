#include "experiments/btree/rope.h"

#include <cstddef>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace btree {

namespace {

// Four lines of 9, 9, 10, and 12 bytes, ending in a newline: 44 bytes, 4 newlines.
constexpr std::string_view poem = "the quick\nbrown fox\njumps over\nthe lazy dog\n";

std::string repeat(std::string_view piece, size_t times) {
    std::string out;
    for (size_t i = 0; i < times; i++) out += piece;
    return out;
}

std::vector<size_t> chunk_sizes(const Rope& rope) {
    std::vector<size_t> sizes;
    for (const Chunk& chunk : rope.chunks()) sizes.push_back(chunk.text().size());
    return sizes;
}

// Line and column of a byte offset, by scanning.
Point naive_point(std::string_view text, size_t offset) {
    Point p;
    for (size_t i = 0; i < std::min(offset, text.size()); i++) {
        if (text[i] == '\n') {
            p.line++;
            p.column = 0;
        } else {
            p.column++;
        }
    }
    return p;
}

// Where a line begins, by scanning; text.size() for a line past the last.
size_t naive_line_start(std::string_view text, size_t line) {
    size_t at = 0;
    for (; line > 0; line--) {
        at = text.find('\n', at);
        if (at == std::string_view::npos) return text.size();
        at++;
    }
    return at;
}

}  // namespace

TEST(RopeTest, EmptyRopeHasOneEmptyLine) {
    Rope rope;
    EXPECT_TRUE(rope.empty());
    EXPECT_EQ(rope.size(), 0u);
    EXPECT_EQ(rope.line_count(), 1u);
    EXPECT_EQ(rope.str(), "");
    EXPECT_EQ(rope.offset_to_point(0), (Point{0, 0}));
    EXPECT_EQ(rope.line_start(0), 0u);
    EXPECT_EQ(rope.line_length(0), 0u);
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, SummarizeCountsBytesLinesAndLineLengths) {
    EXPECT_EQ(summarize(""), (TextSummary{0, 0, 0, 0, 0}));
    EXPECT_EQ(summarize("abc"), (TextSummary{3, 0, 3, 3, 3}));
    EXPECT_EQ(summarize("ab\n"), (TextSummary{3, 1, 2, 0, 2}));
    EXPECT_EQ(summarize("ab\ncdef\ng"), (TextSummary{9, 2, 2, 1, 4}));
}

// Adding summaries joins the last line of the left piece to the first line of the right one.
TEST(RopeTest, SummariesAddAcrossAStraddlingLine) {
    TextSummary left = summarize("ab\ncd");
    TextSummary right = summarize("efgh\ni");
    EXPECT_EQ(left + right, summarize("ab\ncdefgh\ni"));
    EXPECT_EQ((left + right).longest_line, 6u);  // "cdefgh" spans both pieces.
    EXPECT_EQ(summarize("abc") + summarize("def"), summarize("abcdef"));
    EXPECT_EQ(TextSummary{} + right, right);
    EXPECT_EQ(left + TextSummary{}, left);
}

TEST(RopeTest, ConstructsFromTextInChunksOfAtMost128Bytes) {
    std::string text = repeat("0123456789", 50);  // 500 bytes.
    Rope rope(text);
    EXPECT_EQ(rope.str(), text);
    EXPECT_EQ(rope.size(), 500u);
    EXPECT_EQ(chunk_sizes(rope), (std::vector<size_t>{125, 125, 125, 125}));
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, CutsOnlyAtCodePointBoundaries) {
    std::string text = repeat("é", 100);  // 200 bytes of two-byte code points.
    Rope rope(text);
    EXPECT_EQ(rope.str(), text);
    for (size_t size : chunk_sizes(rope)) EXPECT_EQ(size % 2, 0u) << size;
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, InsertsAtTheStartMiddleAndEnd) {
    Rope rope("bd");
    rope.insert(0, "a");
    rope.insert(2, "c");
    rope.insert(4, "e");
    EXPECT_EQ(rope.str(), "abcde");
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, InsertIntoEmptyRopeAndPastTheEndClamps) {
    Rope rope;
    rope.insert(0, "abc");
    rope.insert(99, "d");
    EXPECT_EQ(rope.str(), "abcd");
}

TEST(RopeTest, InsertSplitsAFullChunk) {
    std::string text = repeat("x", 128);
    Rope rope(text);
    EXPECT_EQ(chunk_sizes(rope), (std::vector<size_t>{128}));
    rope.insert(64, "y");
    EXPECT_EQ(rope.str(), repeat("x", 64) + "y" + repeat("x", 64));
    EXPECT_EQ(chunk_sizes(rope), (std::vector<size_t>{65, 64}));
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, ErasesWithinAChunkAcrossChunksAndEverything) {
    std::string text = repeat("0123456789", 50);
    Rope rope(text);
    rope.erase(3, 7);
    text.erase(3, 4);
    EXPECT_EQ(rope.str(), text);
    rope.erase(100, 300);  // Spans whole chunks.
    text.erase(100, 200);
    EXPECT_EQ(rope.str(), text);
    EXPECT_TRUE(rope.check_invariants());
    rope.erase(0, rope.size());
    EXPECT_TRUE(rope.empty());
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, EraseMergesTheChunksItLeavesSmall) {
    Rope rope(repeat("0123456789", 50));  // Four chunks of 125.
    rope.erase(10, 490);                  // 10 bytes survive at each end, in two chunks.
    EXPECT_EQ(rope.str(), "0123456789" + std::string("0123456789"));
    EXPECT_EQ(chunk_sizes(rope), (std::vector<size_t>{20}));
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, SubstrReadsAcrossChunks) {
    std::string text = repeat("0123456789", 50);
    Rope rope(text);
    EXPECT_EQ(rope.substr(120, 130), text.substr(120, 10));
    EXPECT_EQ(rope.substr(0, 500), text);
    EXPECT_EQ(rope.substr(495, 999), text.substr(495));
    EXPECT_EQ(rope.substr(7, 7), "");
}

TEST(RopeTest, OffsetToPointAndBack) {
    Rope rope(poem);
    EXPECT_EQ(rope.offset_to_point(0), (Point{0, 0}));
    EXPECT_EQ(rope.offset_to_point(9), (Point{0, 9}));   // The first newline.
    EXPECT_EQ(rope.offset_to_point(10), (Point{1, 0}));  // "brown".
    EXPECT_EQ(rope.offset_to_point(25), (Point{2, 5}));
    EXPECT_EQ(rope.offset_to_point(44), (Point{4, 0}));  // The end, after the last newline.
    EXPECT_EQ(rope.offset_to_point(99), (Point{4, 0}));
    EXPECT_EQ(rope.point_to_offset({2, 5}), 25u);
    EXPECT_EQ(rope.point_to_offset({2, 99}), 30u);  // Clamped to the line's end.
    EXPECT_EQ(rope.point_to_offset({9, 0}), 44u);   // Past the last line.
}

TEST(RopeTest, LineStartsLengthsAndCount) {
    Rope rope(poem);
    EXPECT_EQ(rope.line_count(), 5u);  // The newline at the end opens an empty fifth line.
    EXPECT_EQ(rope.line_start(0), 0u);
    EXPECT_EQ(rope.line_start(2), 20u);
    EXPECT_EQ(rope.line_start(4), 44u);
    EXPECT_EQ(rope.line_start(5), 44u);
    EXPECT_EQ(rope.line_length(0), 9u);
    EXPECT_EQ(rope.line_length(3), 12u);
    EXPECT_EQ(rope.line_length(4), 0u);
    EXPECT_EQ(rope.longest_line(), 12u);
}

TEST(RopeTest, LineReturnsTheLineWithoutItsNewline) {
    Rope rope(poem);
    EXPECT_EQ(rope.line(0), "the quick");
    EXPECT_EQ(rope.line(3), "the lazy dog");
    EXPECT_EQ(rope.line(4), "");  // The empty line after the final newline.
    EXPECT_EQ(rope.line(9), "");  // Past the last line.
    Rope straddling("a\n" + repeat("x", 300) + "\nb");
    EXPECT_EQ(straddling.line(1), repeat("x", 300));
    EXPECT_EQ(straddling.line(2), "b");
}

TEST(RopeTest, InsertsInPlaceWhenTheChunkHasRoom) {
    Rope rope("abcd");
    rope.insert(2, "XY");
    EXPECT_EQ(rope.str(), "abXYcd");
    EXPECT_EQ(chunk_sizes(rope), (std::vector<size_t>{6}));
    EXPECT_TRUE(rope.check_invariants());
}

TEST(RopeTest, LongestLineCanStraddleChunks) {
    std::string text = "a\n" + repeat("x", 300) + "\nb";  // The long line spans three chunks.
    Rope rope(text);
    EXPECT_GT(chunk_sizes(rope).size(), 2u);
    EXPECT_EQ(rope.longest_line(), 300u);
    EXPECT_EQ(rope.line_start(2), 303u);
}

// Random inserts and erases of ASCII text agree with a std::string at every step, and the chunk
// invariants hold throughout. The rope starts several chunks long so that edits span whole
// chunks, and positions index into the current text, so edits land anywhere.
void EditsMatchStringOracle(const std::vector<std::tuple<bool, int, int, std::string>>& ops) {
    std::string expected = repeat("lorem ipsum\n", 50);  // 600 bytes.
    Rope rope(expected);
    for (const auto& [is_insert, position, length, text] : ops) {
        size_t size = expected.size();
        if (is_insert) {
            size_t at = static_cast<size_t>(position) % (size + 1);
            rope.insert(at, text);
            expected.insert(at, text);
        } else if (size > 0) {
            size_t from = static_cast<size_t>(position) % size;
            size_t to = std::min(size, from + static_cast<size_t>(length));
            rope.erase(from, to);
            expected.erase(from, to - from);
        }
        ASSERT_TRUE(rope.check_invariants()) << (is_insert ? "insert" : "erase");
        ASSERT_EQ(rope.size(), expected.size());
        ASSERT_EQ(rope.str(), expected);
    }
    // Every line and offset converts as a scan of the text would.
    for (size_t offset = 0; offset <= expected.size(); offset += 7) {
        EXPECT_EQ(rope.offset_to_point(offset), naive_point(expected, offset)) << offset;
    }
    for (size_t line = 0; line <= rope.line_count(); line++) {
        EXPECT_EQ(rope.line_start(line), naive_line_start(expected, line)) << line;
    }
}
FUZZ_TEST(RopeFuzzTest, EditsMatchStringOracle)
    .WithDomains(fuzztest::VectorOf(fuzztest::TupleOf(
        fuzztest::Arbitrary<bool>(),
        fuzztest::InRange(0, 2000),
        fuzztest::InRange(0, 300),
        fuzztest::StringOf(fuzztest::ElementOf({'a', 'b', ' ', '\n'})).WithMaxSize(400))));

}  // namespace btree
