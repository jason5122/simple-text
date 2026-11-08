#include "experiments/btree/rope.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace btree {

namespace {

// A UTF-8 continuation byte, which can never begin a chunk or a code point.
bool is_continuation(char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }

// Cuts text into chunks of at most Chunk::max_size bytes, as evenly as possible so that none is
// left tiny, moving each cut back to the start of a code point.
std::vector<Chunk> cut(std::string_view text) {
    std::vector<Chunk> chunks;
    size_t pieces = (text.size() + Chunk::max_size - 1) / Chunk::max_size;
    while (!text.empty()) {
        size_t end = std::min(Chunk::max_size, (text.size() + pieces - 1) / pieces);
        while (end < text.size() && is_continuation(text[end])) end--;
        chunks.emplace_back(text.substr(0, end));
        text.remove_prefix(end);
        if (pieces > 1) pieces--;
    }
    return chunks;
}

}  // namespace

TextSummary summarize(std::string_view text) {
    TextSummary s{.bytes = text.size()};
    size_t line_start = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] != '\n') continue;
        size_t length = i - line_start;
        if (s.lines == 0) s.first_line = length;
        s.longest_line = std::max(s.longest_line, length);
        s.lines++;
        line_start = i + 1;
    }
    size_t length = text.size() - line_start;
    if (s.lines == 0) s.first_line = length;
    s.last_line = length;
    s.longest_line = std::max(s.longest_line, length);
    return s;
}

Chunk::Chunk(std::string_view text) : size_{static_cast<uint8_t>(text.size())} {
    std::copy(text.begin(), text.end(), bytes_.begin());
}

void Chunk::insert(size_t at, std::string_view text) {
    std::copy_backward(bytes_.begin() + at, bytes_.begin() + size_,
                       bytes_.begin() + size_ + text.size());
    std::copy(text.begin(), text.end(), bytes_.begin() + at);
    size_ = static_cast<uint8_t>(size_ + text.size());
}

void Chunk::erase(size_t from, size_t to) {
    std::copy(bytes_.begin() + to, bytes_.begin() + size_, bytes_.begin() + from);
    size_ = static_cast<uint8_t>(size_ - (to - from));
}

Rope::Rope(std::string_view text) : chunks_{cut(text)} {}

void Rope::insert(size_t offset, std::string_view text) {
    if (text.empty()) return;
    offset = std::min(offset, size());
    if (empty()) {
        chunks_ = Chunks(cut(text));
        return;
    }
    // The chunk whose end reaches the offset: an insert at a chunk boundary extends the chunk on
    // the left, and an insert at the very end extends the last chunk.
    Chunks::iterator it =
        chunks_.seek([offset](const TextSummary& s) { return s.bytes >= offset; });
    size_t start = it.start().bytes;
    if (it->text().size() + text.size() <= Chunk::max_size) {
        // Room in the chunk: insert in place. A chunk that grew cannot newly fit a neighbour, so
        // there is nothing to coalesce.
        Chunk chunk = *it;
        chunk.insert(offset - start, text);
        chunks_.replace(it, chunk);
        return;
    }
    std::string joined(it->text().substr(0, offset - start));
    joined += text;
    joined += it->text().substr(offset - start);
    put(it, joined);
    coalesce(start, start + joined.size());
}

void Rope::erase(size_t from, size_t to) {
    to = std::min(to, size());
    if (from >= to) return;
    Chunks::iterator it = chunk_at(from);
    size_t start = it.start().bytes;
    if (to <= start + it->text().size()) {
        // The range lies inside one chunk: erase in place, then see whether the smaller chunk
        // fits a neighbour.
        Chunk chunk = *it;
        chunk.erase(from - start, to - start);
        if (chunk.text().empty()) {
            chunks_.erase(it);
        } else {
            chunks_.replace(it, chunk);
        }
        coalesce(start, start + chunk.text().size());
        return;
    }
    std::string joined(it->text().substr(0, from - start));  // What survives before the range.
    while (true) {
        size_t chunk_start = it.start().bytes;
        size_t end = chunk_start + it->text().size();
        if (end > to) {
            // This chunk reaches past the range: its tail survives, after the saved head.
            joined += it->text().substr(to - chunk_start);
            put(it, joined);
            break;
        }
        // The chunk ends inside the range, so none of it survives. Erasing it moves everything
        // after it left by its size, and the range's end with it.
        bool at_boundary = end == to;
        it = chunks_.erase(it);
        to -= end - chunk_start;
        if (at_boundary) {
            // The range ended at the chunk's end: the saved head goes back before what follows.
            std::vector<Chunk> chunks = cut(joined);
            chunks_.insert(it, chunks.begin(), chunks.end());
            break;
        }
    }
    coalesce(start, start + joined.size());
}

std::string Rope::str() const {
    std::string out;
    out.reserve(size());
    for (const Chunk& chunk : chunks_) out += chunk.text();
    return out;
}

std::string Rope::substr(size_t from, size_t to) const {
    to = std::min(to, size());
    std::string out;
    if (from < to) out.reserve(to - from);
    for (Chunks::iterator it = chunk_at(from), end = chunks_.end();
         it != end && it.start().bytes < to; ++it) {
        size_t start = it.start().bytes;
        size_t lo = std::max(from, start) - start;
        size_t hi = std::min(to, start + it->text().size()) - start;
        out += it->text().substr(lo, hi - lo);
    }
    return out;
}

Point Rope::offset_to_point(size_t offset) const {
    if (offset >= size()) return {summary().lines, summary().last_line};
    // Everything before the offset is the chunk's start plus the chunk's text before it, and of
    // that text only its newlines and the bytes after the last one matter.
    Chunks::iterator it = chunk_at(offset);
    std::string_view head = it->text().substr(0, offset - it.start().bytes);
    size_t newline = head.rfind('\n');
    TextSummary before = it.start();
    if (newline == std::string_view::npos) return {before.lines, before.last_line + head.size()};
    return {before.lines + static_cast<size_t>(std::ranges::count(head, '\n')),
            head.size() - newline - 1};
}

size_t Rope::point_to_offset(Point point) const {
    LineStart at = line_at(point.line);
    if (at.chunk == chunks_.end()) return size();
    // The column is clamped to the line, which takes a walk only as far as the column itself.
    return at.chunk.start().bytes + at.pos +
           std::min(point.column, walk_line(at, point.column, nullptr));
}

size_t Rope::line_start(size_t line) const {
    LineStart at = line_at(line);
    return at.chunk == chunks_.end() ? size() : at.chunk.start().bytes + at.pos;
}

size_t Rope::line_length(size_t line) const {
    return walk_line(line_at(line), std::numeric_limits<size_t>::max(), nullptr);
}

std::string Rope::line(size_t line) const {
    std::string out;
    walk_line(line_at(line), std::numeric_limits<size_t>::max(), &out);
    return out;
}

bool Rope::check_invariants() const {
    if (!chunks_.check_invariants()) return false;
    Chunks::iterator end = chunks_.end();
    for (Chunks::iterator it = chunks_.begin(); it != end; ++it) {
        std::string_view text = it->text();
        if (text.empty() || text.size() > Chunk::max_size || is_continuation(text[0])) {
            return false;
        }
        Chunks::iterator next = it;
        ++next;
        if (next != end && text.size() + next->text().size() <= Chunk::max_size) {
            return false;
        }
    }
    return true;
}

Rope::Chunks::iterator Rope::chunk_at(size_t offset) const {
    return chunks_.seek([offset](const TextSummary& s) { return s.bytes > offset; });
}

Rope::LineStart Rope::line_at(size_t line) const {
    if (line == 0) return {chunks_.begin(), 0};
    // The line begins after the line-th newline, which is in the first chunk whose newline count
    // reaches that many.
    Chunks::iterator it = chunks_.seek([line](const TextSummary& s) { return s.lines >= line; });
    size_t pos = 0;
    if (it != chunks_.end()) {
        for (size_t k = line - it.start().lines; k > 0; k--) {
            pos = it->text().find('\n', pos) + 1;
        }
    }
    return {it, pos};
}

// Walks from a line's start to its newline, the end of the text, or `limit` bytes, whichever
// comes first, returning the bytes walked and appending them to `out` if given.
size_t Rope::walk_line(LineStart from, size_t limit, std::string* out) const {
    size_t bytes = 0;
    size_t pos = from.pos;
    for (Chunks::iterator it = from.chunk, end = chunks_.end(); it != end && bytes < limit;
         ++it, pos = 0) {
        std::string_view text = it->text().substr(pos);
        size_t newline = text.find('\n');
        if (newline != std::string_view::npos) text = text.substr(0, newline);
        bytes += text.size();
        if (out != nullptr) *out += text;
        if (newline != std::string_view::npos) break;
    }
    return bytes;
}

// Replaces the chunk at `at` with `text`, cut into chunks, or removes it if the text is empty.
void Rope::put(Chunks::iterator at, std::string_view text) {
    std::vector<Chunk> chunks = cut(text);
    if (chunks.empty()) {
        chunks_.erase(at);
        return;
    }
    chunks_.replace(at, chunks[0]);
    Chunks::iterator next = at;
    ++next;
    chunks_.insert(next, chunks.begin() + 1, chunks.end());
}

// Merges neighbouring chunks that would fit in one, from the chunk before byte `from` through
// the chunk holding byte `to`: the stretch an edit between those bytes can have left too small.
void Rope::coalesce(size_t from, size_t to) {
    Chunks::iterator it = chunk_at(from == 0 ? 0 : from - 1);
    while (it != chunks_.end() && it.start().bytes <= to) {
        Chunks::iterator next = it;
        ++next;
        if (next == chunks_.end()) break;
        if (it->text().size() + next->text().size() > Chunk::max_size) {
            it = next;
            continue;
        }
        std::string joined(it->text());
        joined += next->text();
        chunks_.replace(next, Chunk(joined));
        it = chunks_.erase(it);  // The position after the erased chunk: the merged one.
    }
}

}  // namespace btree
