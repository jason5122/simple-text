#include "base/numeric/saturation_arithmetic.h"
#include "piece_table.h"
#include "util/escape_special_chars.h"
#include <format>
#include <iostream>

namespace base {

PieceTable::PieceTable(std::string_view str) : original{str}, m_length{str.length()} {
    pieces.emplace_front(Piece{
        .source = PieceSource::Original,
        .start = 0,
        .length = str.length(),
        .line_starts = cacheLineStarts(str),
    });
}

void PieceTable::insert(size_t index, std::string_view str) {
    if (index > length()) {
        std::cerr << "PieceTable::insert() out of range error: index > length()\n";
        std::abort();
    }

    auto it = pieces.begin();
    size_t offset = 0;
    while (it != pieces.end()) {
        size_t piece_start = offset;
        size_t piece_end = offset + (*it).length;
        if (piece_start <= index && index <= piece_end) {
            break;
        }
        offset += (*it).length;
        it++;
    }

    if (it == pieces.end()) [[unlikely]] {
        std::cerr << "PieceTable::insert() unknown error: index <= length(), but there was no "
                     "corresponding piece\n";
        std::abort();
    }

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
        const Piece piece{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
            .line_starts = cacheLineStarts(str),
        };
        auto pos = index == piece_start ? it : std::next(it);
        pieces.insert(pos, piece);
    }
    // Case 2: Middle of a piece. We need to split one piece into three pieces.
    else {
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        // Split up line starts, if necessary.
        std::list<size_t> p3_line_starts;
        for (auto it = p1.line_starts.begin(); it != p1.line_starts.end(); it++) {
            if ((*it) >= p1.length) {
                p3_line_starts.splice(p3_line_starts.begin(), p1.line_starts, it,
                                      p1.line_starts.end());
                break;
            }
        }
        for (auto& line_start : p3_line_starts) {
            line_start -= p1.length;
        }

        const Piece p2{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
            .line_starts = cacheLineStarts(str),
        };
        const Piece p3{
            .source = p1.source,
            .start = p1.start + p1.length,
            .length = p1_old_length - p1.length,
            .line_starts = std::move(p3_line_starts),
        };
        auto it2 = pieces.insert(std::next(it), p2);
        pieces.insert(std::next(it2), p3);
    }
}

void PieceTable::erase(size_t index, size_t count) {
    if (index > length()) {
        std::cerr << "PieceTable::erase() out of range error: index > length()\n";
        std::abort();
    }

    auto it = pieces.begin();
    size_t offset = 0;
    while (it != pieces.end()) {
        size_t piece_start = offset;
        size_t piece_end = offset + (*it).length;
        if (piece_start <= index && index <= piece_end) {
            break;
        }
        offset += (*it).length;
        it++;
    }

    if (it == pieces.end()) [[unlikely]] {
        std::cerr << "PieceTable::erase() unknown error: index <= length(), but there was no "
                     "corresponding piece\n";
        std::abort();
    }

    Piece& p1 = *it;
    size_t piece_start = offset;
    size_t piece_end = offset + (*it).length;

    // Case 1: Middle of a piece. We need to split one piece into two pieces.
    if (piece_start <= index && index + count <= piece_end) {
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        // Split up line starts, if necessary.
        std::list<size_t> p2_line_starts;
        for (auto it = p1.line_starts.begin(); it != p1.line_starts.end(); it++) {
            if ((*it) >= p1.length) {
                p2_line_starts.splice(p2_line_starts.begin(), p1.line_starts, it,
                                      p1.line_starts.end());
                break;
            }
        }
        // Remove erased line starts.
        size_t old_p2_size = p2_line_starts.size();
        p2_line_starts.remove_if([&](auto line_start) { return line_start < p1.length + count; });
        size_t num_removed = old_p2_size - p2_line_starts.size();
        // Decrement the line count.
        m_line_count = base::sub_sat(m_line_count, num_removed);
        // Adjust the remaining line starts after the split.
        for (auto& line_start : p2_line_starts) {
            line_start -= p1.length + count;
        }

        const Piece p2{
            .source = p1.source,
            .start = p1.start + p1.length + count,
            .length = p1_old_length - (p1.length + count),
            .line_starts = std::move(p2_line_starts),
        };
        pieces.insert(std::next(it), p2);

        m_length -= count;
    }
    // Case 2: Spanning multiple pieces.
    else {
        // Erase from the first piece without updating the piece's start.
        size_t sub = std::min(piece_end - index, count);
        p1.length -= sub;
        count -= sub;
        m_length -= sub;
        it++;

        // Remove erased line starts.
        size_t old_size = p1.line_starts.size();
        p1.line_starts.remove_if([&](auto line_start) { return line_start >= p1.length; });
        size_t num_removed = old_size - p1.line_starts.size();
        // Decrement the line count.
        m_line_count = base::sub_sat(m_line_count, num_removed);

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
            size_t old_size = piece.line_starts.size();
            piece.line_starts.remove_if([&](auto line_start) { return line_start < sub; });
            size_t num_removed = old_size - piece.line_starts.size();
            // Decrement the line count.
            m_line_count = base::sub_sat(m_line_count, num_removed);
            // Adjust the remaining line starts after the shift.
            for (auto& line_start : piece.line_starts) {
                line_start -= sub;
            }

            offset += (*it).length;
            it++;
        }
    }
}

size_t PieceTable::length() {
    return m_length;
}

size_t PieceTable::lineCount() {
    // TODO: Should we add one here?
    return m_line_count + 1;
}

std::string PieceTable::str() {
    std::string result;
    for (const auto& piece : pieces) {
        const std::string& buffer = piece.source == PieceSource::Original ? original : add;
        result += buffer.substr(piece.start, piece.length);
    }
    return result;
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
        std::string line_start_str = "[";
        for (auto it = piece.line_starts.begin(); it != piece.line_starts.end(); it++) {
            line_start_str += std::to_string(*it);
            if (std::next(it) != piece.line_starts.end()) {
                line_start_str += ", ";
            }
        }
        line_start_str += "]";

        out << std::format("Piece(start={}, length={}, source={}, line_starts={}): \"{}\"\n",
                           piece.start, piece.length, source, line_start_str,
                           EscapeSpecialChars(piece_str));
    }
    return out;
}

PieceTable::Iterator PieceTable::begin() {
    auto piece_it = pieces.begin();
    size_t piece_index = 0;

    // Skip past any empty pieces.
    while (piece_it != pieces.end() && piece_index == piece_it->length) {
        piece_index = 0;
        ++piece_it;
    }

    return {*this, piece_it, piece_index};
}

PieceTable::Iterator PieceTable::end() {
    return {*this, pieces.end(), 0};
}

PieceTable::Iterator PieceTable::line(size_t line_index) {
    if (line_index == 0) {
        return begin();
    }
    --line_index;

    size_t count = 0;
    for (auto it = pieces.begin(); it != pieces.end(); it++) {
        size_t n = (*it).line_starts.size();
        if (count + n < line_index) {
            count += n;
        } else {
            for (size_t line_start : (*it).line_starts) {
                if (count == line_index) {
                    return {*this, it, line_start};
                }
                count++;
            }
        }
    }
    return end();
}

PieceTable::Iterator::reference PieceTable::Iterator::operator*() const {
    size_t i = piece_it->start + piece_index;
    std::string& buffer = piece_it->source == PieceSource::Original ? table.original : table.add;
    return buffer[i];
}

PieceTable::Iterator::pointer PieceTable::Iterator::operator->() {
    size_t i = piece_it->start + piece_index;
    std::string& buffer = piece_it->source == PieceSource::Original ? table.original : table.add;
    return &buffer[i];
}

PieceTable::Iterator& PieceTable::Iterator::operator++() {
    ++piece_index;
    // If the end of the piece is reached, move onto the next piece.
    // We use a while loop in order to skip past any empty pieces.
    while (piece_index == piece_it->length) {
        piece_index = 0;
        ++piece_it;
    }
    return *this;
}

PieceTable::Iterator PieceTable::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

bool operator==(const PieceTable::Iterator& a, const PieceTable::Iterator& b) {
    return a.piece_it == b.piece_it && a.piece_index == b.piece_index;
}

bool operator!=(const PieceTable::Iterator& a, const PieceTable::Iterator& b) {
    return a.piece_it != b.piece_it || a.piece_index != b.piece_index;
}

std::ostream& operator<<(std::ostream& out, const PieceTable::Iterator& it) {
    size_t dist = std::distance(it.pieces().begin(), it.piece_it);
    const std::string_view end_str = it.piece_it == it.pieces().end() ? " (end)" : "";
    return out << std::format("PieceTable::Iterator(piece_it = {}{}, piece_index = {})", dist,
                              end_str, it.piece_index);
}

std::list<size_t> PieceTable::cacheLineStarts(std::string_view str) {
    std::list<size_t> line_starts;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\n') {
            line_starts.emplace_back(i);
        }
    }
    m_line_count += line_starts.size();
    return line_starts;
}

}
