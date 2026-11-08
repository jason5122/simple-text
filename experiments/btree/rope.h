#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "experiments/btree/sum_tree.h"

namespace btree {

// A position in text: a line and a byte column within it, both counted from zero.
struct Point {
    size_t line = 0;
    size_t column = 0;
    bool operator==(const Point&) const = default;
};

// The measure of a piece of UTF-8 text. bytes and lines are plain counts. The line fields are
// what make + depend on order: a piece's first and last lines are partial, continued by the
// pieces on either side, and the longest line can straddle several pieces.
struct TextSummary {
    size_t bytes = 0;
    size_t lines = 0;         // Newlines.
    size_t first_line = 0;    // Bytes before the first newline, or all of them if there is none.
    size_t last_line = 0;     // Bytes after the last newline, or all of them if there is none.
    size_t longest_line = 0;  // Bytes in the longest line, the partial first and last included.

    bool operator==(const TextSummary&) const = default;
    friend TextSummary operator+(const TextSummary& a, const TextSummary& b) {
        return {
            .bytes = a.bytes + b.bytes,
            .lines = a.lines + b.lines,
            .first_line = a.lines == 0 ? a.first_line + b.first_line : a.first_line,
            .last_line = b.lines == 0 ? a.last_line + b.last_line : b.last_line,
            .longest_line =
                std::max(std::max(a.longest_line, b.longest_line), a.last_line + b.first_line),
        };
    }
};

// The summary of any text, not only a chunk's.
TextSummary summarize(std::string_view text);

// A piece of the text of at most max_size bytes, never split inside a code point.
class Chunk {
public:
    static constexpr size_t max_size = 128;

    Chunk() = default;
    explicit Chunk(std::string_view text);  // text.size() <= max_size.

    std::string_view text() const { return {bytes_.data(), size_}; }
    TextSummary summary() const { return summarize(text()); }

    void insert(size_t at, std::string_view text);  // text().size() + text.size() <= max_size.
    void erase(size_t from, size_t to);

private:
    static_assert(max_size <= UINT8_MAX);
    std::array<char, max_size> bytes_;
    uint8_t size_ = 0;
};

// A text buffer as a SumTree of chunks: byte offsets, lines, and the longest line are found by
// seeking rather than scanning, and an edit costs O(log n) plus the chunks it touches. Offsets
// are bytes and must fall on code point boundaries; lines are separated by '\n'.
class Rope {
public:
    // t = 16 by measurement (rope_perftest.cc): at the default 2 every seek walked four times as
    // many levels and ran two to three times slower than the old rope; 16 beats 8 on every
    // lookup and read by 25-50% for about 20% on typing and pastes, still under a microsecond.
    using Chunks = SumTree<Chunk, 16>;

    Rope() = default;
    explicit Rope(std::string_view text);

    size_t size() const { return summary().bytes; }  // Bytes.
    bool empty() const { return chunks_.empty(); }
    size_t line_count() const { return summary().lines + 1; }
    size_t longest_line() const { return summary().longest_line; }  // Bytes.
    TextSummary summary() const { return chunks_.summary(); }

    void insert(size_t offset, std::string_view text);
    void erase(size_t from, size_t to);  // The bytes in [from, to).

    std::string str() const;
    std::string substr(size_t from, size_t to) const;
    std::string line(size_t line) const;  // Without its newline; empty past the last line.

    Point offset_to_point(size_t offset) const;
    size_t point_to_offset(Point point) const;  // Clamped to the line's end.
    size_t line_start(size_t line) const;       // size() for a line past the last.
    size_t line_length(size_t line) const;      // Bytes, without the newline.

    const Chunks& chunks() const { return chunks_; }
    // The chunks are valid, none is empty or over max_size, none starts inside a code point,
    // and no two neighbours would fit in one chunk.
    bool check_invariants() const;

private:
    // The chunk holding the start of a line, and the start's offset within it.
    struct LineStart {
        Chunks::iterator chunk;
        size_t pos;
    };

    Chunks::iterator chunk_at(size_t offset) const;  // The chunk holding byte `offset`.
    LineStart line_at(size_t line) const;            // chunk is end() past the last line.
    size_t walk_line(LineStart from, size_t limit, std::string* out) const;
    void put(Chunks::iterator at, std::string_view text);
    void coalesce(size_t from, size_t to);

    Chunks chunks_;
};

}  // namespace btree
