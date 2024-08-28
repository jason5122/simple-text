#include "base/numeric/literals.h"
#include "base/numeric/saturation_arithmetic.h"
#include "piece_table.h"
#include "util/escape_special_chars.h"

// TODO: Debug use; remove this.
#include "util/profile_util.h"
#include <format>
#include <iostream>

namespace base {

PieceTable::PieceTable(std::string_view str) : original{str}, m_length{str.length()} {
    pieces.emplace_back(Piece{
        .source = PieceSource::Original,
        .start = 0,
        .length = str.length(),
        .newlines = cacheNewlines(str),
    });
}

void PieceTable::insert(size_t index, std::string_view str) {
    // Invalidate line cache.
    line_cache.clear();

    auto [it, offset] = pieceAt(index);

    Piece& p1 = *it;
    size_t piece_start = offset;
    size_t piece_end = offset + (*it).length;

    // Append string to `add` buffer.
    size_t add_start = add.length();
    add += str;

    // Update length.
    m_length += str.length();

    // Case 1: Beginning/end of a piece. We only need to add a single piece.
    if (index == piece_start || index == piece_end) {
        Piece piece{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
            .newlines = cacheNewlines(str),
        };
        auto pos = index == piece_start ? it : std::next(it);
        pieces.insert(pos, std::move(piece));
    }
    // Case 2: Middle of a piece. We need to split one piece into three pieces.
    else {
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        // Split up line starts, if necessary.
        std::list<size_t> p3_newlines;
        for (auto it = p1.newlines.begin(); it != p1.newlines.end(); ++it) {
            if ((*it) >= p1.length) {
                p3_newlines.splice(p3_newlines.begin(), p1.newlines, it, p1.newlines.end());
                break;
            }
        }
        for (auto& newline : p3_newlines) {
            newline -= p1.length;
        }

        Piece p2{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
            .newlines = cacheNewlines(str),
        };
        Piece p3{
            .source = p1.source,
            .start = p1.start + p1.length,
            .length = p1_old_length - p1.length,
            .newlines = std::move(p3_newlines),
        };
        auto it2 = pieces.insert(std::next(it), std::move(p2));
        pieces.insert(std::next(it2), std::move(p3));
    }
}

void PieceTable::erase(size_t index, size_t count) {
    // Invalidate line cache.
    line_cache.clear();

    auto [it, offset] = pieceAt(index);

    Piece& p1 = *it;
    size_t piece_start = offset;
    size_t piece_end = offset + (*it).length;

    // Case 1: Middle of a piece. We need to split one piece into two pieces.
    if (piece_start <= index && index + count <= piece_end) {
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        // Split up line starts, if necessary.
        std::list<size_t> p2_newlines;
        for (auto it = p1.newlines.begin(); it != p1.newlines.end(); ++it) {
            if ((*it) >= p1.length) {
                p2_newlines.splice(p2_newlines.begin(), p1.newlines, it, p1.newlines.end());
                break;
            }
        }

        // Remove erased line starts.
        size_t old_p2_size = p2_newlines.size();
        p2_newlines.remove_if([&](auto newline) { return newline < p1.length + count; });
        size_t num_removed = old_p2_size - p2_newlines.size();
        // Decrement the line count.
        newline_count = base::sub_sat(newline_count, num_removed);
        // Adjust the remaining line starts after the split.
        for (auto& newline : p2_newlines) {
            newline -= p1.length + count;
        }

        Piece p2{
            .source = p1.source,
            .start = p1.start + p1.length + count,
            .length = p1_old_length - (p1.length + count),
            .newlines = std::move(p2_newlines),
        };
        pieces.insert(std::next(it), std::move(p2));

        m_length -= count;
    }
    // Case 2: Spanning multiple pieces.
    else {
        // Erase from the first piece without updating the piece's start.
        size_t sub = std::min(piece_end - index, count);
        p1.length -= sub;
        count -= sub;
        m_length -= sub;
        ++it;

        // Remove erased line starts.
        size_t old_size = p1.newlines.size();
        p1.newlines.remove_if([&](auto newline) { return newline >= p1.length; });
        size_t num_removed = old_size - p1.newlines.size();
        // Decrement the line count.
        newline_count = base::sub_sat(newline_count, num_removed);

        // Erase from next pieces until `count` is exhausted, updating each piece's start.
        while (it != pieces.end() && count > 0) {
            Piece& piece = *it;
            size_t piece_start = offset;
            size_t piece_end = offset + (*it).length;

            size_t sub = std::min(piece_end - piece_start, count);
            piece.start += sub;
            piece.length -= sub;
            count -= sub;
            m_length -= sub;

            // Remove erased line starts.
            size_t old_size = piece.newlines.size();
            piece.newlines.remove_if([&](auto newline) { return newline < sub; });
            size_t num_removed = old_size - piece.newlines.size();
            // Decrement the line count.
            newline_count = base::sub_sat(newline_count, num_removed);
            // Adjust the remaining line starts after the shift.
            for (auto& newline : piece.newlines) {
                newline -= sub;
            }

            offset += (*it).length;
            ++it;
        }
    }
}

size_t PieceTable::length() const {
    return m_length;
}

size_t PieceTable::lineCount() const {
    return newline_count + 1;
}

size_t PieceTable::newlineCount() const {
    return newline_count;
}

std::string PieceTable::line(size_t index) {
    if (index > newlineCount()) {
        std::cerr << "PieceTable::line() out of range error: index > newlineCount()\n";
        std::abort();
    }

    // Memoize line strings.
    if (line_cache.contains(index)) {
        return line_cache.at(index);
    }

    auto first = index == 0 ? begin() : std::next(newline(base::sub_sat(index, 1_Z)));
    auto last = index == newlineCount() ? end() : std::next(newline(index));

    std::string line_str;
    for (auto it = first; it != last; ++it) {
        line_str += *it;
    }

    line_cache.emplace(index, line_str);
    return line_str;
}

std::string PieceTable::str() const {
    std::string result;
    for (const auto& piece : pieces) {
        const std::string& buffer = piece.source == PieceSource::Original ? original : add;
        result += buffer.substr(piece.start, piece.length);
    }
    return result;
}

std::string PieceTable::substr(size_t index, size_t count) const {
    auto [it, offset] = pieceAt(index);

    const Piece& p1 = *it;
    size_t piece_start = offset;
    size_t piece_end = offset + (*it).length;

    // Case 1: Middle of a piece.
    if (piece_start <= index && index + count <= piece_end) {
        const std::string& buffer = p1.source == PieceSource::Original ? original : add;
        return buffer.substr(p1.start + (index - offset), count);
    }
    // Case 2: Spanning multiple pieces.
    else {
        std::string str;

        size_t sub = std::min(piece_end - index, count);
        count -= sub;
        const std::string& buffer = p1.source == PieceSource::Original ? original : add;
        str += buffer.substr(p1.start + (p1.length - sub), sub);

        ++it;

        while (it != pieces.end() && count > 0) {
            const Piece& piece = *it;
            size_t piece_start = offset;
            size_t piece_end = offset + (*it).length;

            size_t sub = std::min(piece_end - piece_start, count);
            count -= sub;
            const std::string& buffer = piece.source == PieceSource::Original ? original : add;
            str += buffer.substr(piece.start, sub);

            offset += (*it).length;
            ++it;
        }
        return str;
    }
}

std::pair<size_t, size_t> PieceTable::lineColumnAt(size_t index) const {
    PROFILE_BLOCK("PieceTable::lineColumnAt()");

    if (index > m_length) {
        index = m_length;
    }

    size_t prev_line = 0;
    size_t prev_line_start = 0;

    size_t total_len = 0;
    for (const auto& piece : pieces) {
        for (size_t offset : piece.newlines) {
            size_t line_start = total_len + offset + 1;

            if (index < line_start) {
                return {prev_line, index - prev_line_start};
            }

            ++prev_line;
            prev_line_start = line_start;
        }
        total_len += piece.length;
    }
    return {newline_count, index - prev_line_start};
}

// TODO: Add tests for this method.
size_t PieceTable::indexAt(size_t line, size_t col) const {
    const_iterator line_itr = line == 0 ? begin() : std::next(newline(base::sub_sat(line, 1_Z)));
    size_t index = std::distance(begin(), line_itr) + col;
    return index;
}

PieceTable::iterator PieceTable::begin() {
    PieceIterator piece_it = pieces.begin();
    size_t piece_index = 0;

    // Skip past any empty pieces.
    while (piece_it != pieces.end() && piece_index == piece_it->length) {
        piece_index = 0;
        ++piece_it;
    }

    return {*this, piece_it, piece_index};
}

PieceTable::iterator PieceTable::end() {
    return {*this, pieces.end(), 0};
}

// This calls the const overloads for `begin()/end()` since we are in a const method.
PieceTable::const_iterator PieceTable::begin() const {
    PieceConstIterator piece_it = pieces.begin();
    size_t piece_index = 0;

    // Skip past any empty pieces.
    while (piece_it != pieces.end() && piece_index == piece_it->length) {
        piece_index = 0;
        ++piece_it;
    }

    return {*this, piece_it, piece_index};
}

PieceTable::const_iterator PieceTable::end() const {
    return {*this, pieces.end(), 0};
}

PieceTable::const_iterator PieceTable::cbegin() const {
    return begin();
}

PieceTable::const_iterator PieceTable::cend() const {
    return end();
}

PieceTable::iterator PieceTable::newline(size_t index) {
    size_t count = 0;
    for (auto it = pieces.begin(); it != pieces.end(); ++it) {
        size_t n = (*it).newlines.size();
        if (count + n < index) {
            count += n;
        } else {
            for (size_t newline : (*it).newlines) {
                if (count == index) {
                    return {*this, it, newline};
                }
                ++count;
            }
        }
    }
    return end();
}

// This calls the const overloads for `begin()/end()` since we are in a const method.
PieceTable::const_iterator PieceTable::newline(size_t index) const {
    size_t count = 0;
    for (auto it = pieces.begin(); it != pieces.end(); ++it) {
        size_t n = (*it).newlines.size();
        if (count + n < index) {
            count += n;
        } else {
            for (size_t newline : (*it).newlines) {
                if (count == index) {
                    return {*this, it, newline};
                }
                ++count;
            }
        }
    }
    return end();
}

std::pair<PieceTable::PieceIterator, size_t> PieceTable::pieceAt(size_t index) {
    if (index > length()) {
        std::cerr << "PieceTable::pieceAt() out of range error: index > length()\n";
        std::abort();
    }

    auto it = pieces.begin();
    size_t offset = 0;
    while (it != pieces.end()) {
        size_t piece_start = offset;
        size_t piece_end = offset + (*it).length;
        if (piece_start <= index && index <= piece_end) {
            return {it, offset};
        }
        offset += (*it).length;
        ++it;
    }

    std::cerr << "PieceTable::pieceAt() internal error: index <= length(), but there was no "
                 "corresponding piece\n";
    std::abort();
}

// TODO: Remove code duplication.
std::pair<PieceTable::PieceConstIterator, size_t> PieceTable::pieceAt(size_t index) const {
    if (index > length()) {
        std::cerr << "PieceTable::pieceAt() out of range error: index > length()\n";
        std::abort();
    }

    auto it = pieces.begin();
    size_t offset = 0;
    while (it != pieces.end()) {
        size_t piece_start = offset;
        size_t piece_end = offset + (*it).length;
        if (piece_start <= index && index <= piece_end) {
            return {it, offset};
        }
        offset += (*it).length;
        ++it;
    }

    std::cerr << "PieceTable::pieceAt() internal error: index <= length(), but there was no "
                 "corresponding piece\n";
    std::abort();
}

std::list<size_t> PieceTable::cacheNewlines(std::string_view str) {
    std::list<size_t> newlines;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\n') {
            newlines.emplace_back(i);
        }
    }
    newline_count += newlines.size();
    return newlines;
}

std::ostream& operator<<(std::ostream& out, const PieceTable& table) {
    out << std::format("Original: \"{}\"\n", EscapeSpecialChars(table.original));
    out << std::format("Add: \"{}\"\n", EscapeSpecialChars(table.add));

    for (const auto& piece : table.pieces) {
        const bool is_original = piece.source == PieceTable::PieceSource::Original;
        const std::string& buffer = is_original ? table.original : table.add;
        const std::string source = is_original ? "Original" : "Add";
        const std::string piece_str = buffer.substr(piece.start, piece.length);

        // TODO: Simplify this by implementing `std::format` support for `std::list` elsewhere.
        std::string newlines_str = "[";
        for (auto it = piece.newlines.begin(); it != piece.newlines.end(); ++it) {
            newlines_str += std::to_string(*it);
            if (std::next(it) != piece.newlines.end()) {
                newlines_str += ", ";
            }
        }
        newlines_str += "]";

        out << std::format("Piece(start={}, length={}, source={}, newlines={}): \"{}\"\n",
                           piece.start, piece.length, source, newlines_str,
                           EscapeSpecialChars(piece_str));
    }
    return out;
}

PieceTable::ConstIterator::ConstIterator(const PieceTable& table,
                                         PieceConstIterator piece_it,
                                         size_t piece_index)
    : table{table}, piece_it{piece_it}, piece_index{piece_index} {}

PieceTable::ConstIterator::reference PieceTable::ConstIterator::operator*() const {
    size_t i = piece_it->start + piece_index;
    const std::string& buffer =
        piece_it->source == PieceTable::PieceSource::Original ? table.original : table.add;
    return buffer[i];
}

PieceTable::ConstIterator::pointer PieceTable::ConstIterator::operator->() {
    size_t i = piece_it->start + piece_index;
    const std::string& buffer =
        piece_it->source == PieceTable::PieceSource::Original ? table.original : table.add;
    return &buffer[i];
}

PieceTable::ConstIterator& PieceTable::ConstIterator::operator++() {
    ++piece_index;
    // If the end of the piece is reached, move onto the next piece.
    // We use a while loop in order to skip past any empty pieces.
    while (piece_it != table.pieces.end() && piece_index == piece_it->length) {
        piece_index = 0;
        ++piece_it;
    }
    return *this;
}

PieceTable::ConstIterator PieceTable::ConstIterator::operator++(int) {
    ConstIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool operator==(const PieceTable::ConstIterator& a, const PieceTable::ConstIterator& b) {
    return a.piece_it == b.piece_it && a.piece_index == b.piece_index;
}

bool operator!=(const PieceTable::ConstIterator& a, const PieceTable::ConstIterator& b) {
    return a.piece_it != b.piece_it || a.piece_index != b.piece_index;
}

std::ostream& operator<<(std::ostream& out, const PieceTable::ConstIterator& it) {
    size_t dist = std::distance(it.table.pieces.begin(), it.piece_it);
    const std::string_view end_str = it.piece_it == it.table.pieces.end() ? " (end)" : "";
    return out << std::format("PieceTable::ConstIterator(piece_it = {}{}, piece_index = {})", dist,
                              end_str, it.piece_index);
}

PieceTable::Iterator::Iterator(PieceTable& table, PieceIterator piece_it, size_t piece_index)
    : ConstIterator{table, piece_it, piece_index}, table{table} {}

PieceTable::Iterator::reference PieceTable::Iterator::operator*() const {
    size_t i = piece_it->start + piece_index;
    std::string& buffer =
        piece_it->source == PieceTable::PieceSource::Original ? table.original : table.add;
    return buffer[i];
}

std::ostream& operator<<(std::ostream& out, const PieceTable::Iterator& it) {
    size_t dist = std::distance(it.table.pieces.cbegin(), it.piece_it);
    const std::string_view end_str = it.piece_it == it.table.pieces.end() ? " (end)" : "";
    return out << std::format("PieceTable::Iterator(piece_it = {}{}, piece_index = {})", dist,
                              end_str, it.piece_index);
}

}
