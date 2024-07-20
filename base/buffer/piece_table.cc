#include "piece_table.h"
#include "util/escape_special_chars.h"
#include <format>
#include <iostream>

namespace base {

PieceTable::PieceTable(std::string_view str) : original{str}, _length{str.length()} {
    std::list<size_t> line_starts;

    line_starts.emplace_back(0);
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\n') {
            line_starts.emplace_back(i);
        }
    }
    _line_count = line_starts.size();

    pieces.emplace_front(Piece{
        .source = PieceSource::Original,
        .start = 0,
        .length = str.length(),
        .line_starts = std::move(line_starts),
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

    if (it == pieces.end()) {
        std::cerr << "PieceTable::insert() out of range error: index > length()\n";
        std::abort();
    }

    Piece& p1 = *it;
    size_t piece_start = offset;
    size_t piece_end = offset + (*it).length;

    // Append string to `add` buffer.
    size_t add_start = add.length();
    add += str;

    // Update length.
    _length += str.length();

    // Case 1: Beginning/end of a piece. We only need to add a single piece.
    if (index == piece_start || index == piece_end) {
        const Piece p{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };
        auto pos = index == piece_start ? it : std::next(it);
        pieces.insert(pos, p);
    }
    // Case 2: Middle of a piece. We need to split one piece into three pieces.
    else {
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        const Piece p2{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };
        const Piece p3{
            .source = p1.source,
            .start = p1.start + p1.length,
            .length = p1_old_length - p1.length,
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

    if (it == pieces.end()) {
        std::cerr << "PieceTable::erase() out of range error: index > length()\n";
        std::abort();
    }

    Piece& p1 = *it;
    size_t piece_start = offset;
    size_t piece_end = offset + (*it).length;

    // Case 1 Middle of a piece. We need to split one piece into two pieces.
    if (piece_start <= index && index + count <= piece_end) {
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        const Piece p2{
            .source = p1.source,
            .start = p1.start + p1.length + count,
            .length = p1_old_length - (p1.length + count),
        };
        pieces.insert(std::next(it), p2);

        _length -= count;
    }
    // Case 2: Erase spans multiple pieces.
    else {
        // Erase from the first piece without updating the piece's start.
        size_t sub = std::min(piece_end - index, count);
        p1.length -= sub;
        count -= sub;
        _length -= sub;
        it++;

        // Erase from next pieces until `count` is exhausted, updating each piece's start.
        while (it != pieces.end() && count > 0) {
            Piece& piece = *it;
            size_t piece_start = offset;
            size_t piece_end = offset + (*it).length;

            size_t sub = std::min(piece_end - piece_start, count);
            piece.start += sub;
            piece.length -= sub;
            count -= sub;
            _length -= sub;

            offset += (*it).length;
            it++;
        }
    }
}

size_t PieceTable::length() {
    return _length;
}

size_t PieceTable::lineCount() {
    return _line_count;
}

std::string PieceTable::str() {
    std::string result;
    for (const auto& piece : pieces) {
        const std::string& buffer = piece.source == PieceSource::Original ? original : add;
        result += buffer.substr(piece.start, piece.length);
    }
    return result;
}

std::string PieceTable::lineContent(size_t line_index) {
    size_t curr_index = 0;
    for (auto it = pieces.begin(); it != pieces.end(); it++) {
        for (size_t line_start : (*it).line_starts) {
            curr_index++;
            if (curr_index == line_index + 1) {
                std::cerr << std::format("line_start = {}\n", line_start);
            }
        }
    }
    return "";
}

std::ostream& operator<<(std::ostream& out, const PieceTable& table) {
    out << std::format("Original: \"{}\"\n", EscapeSpecialChars(table.original));
    out << std::format("Add: \"{}\"\n", EscapeSpecialChars(table.add));

    for (const auto& piece : table.pieces) {
        const bool is_original = piece.source == PieceTable::PieceSource::Original;
        const std::string& buffer = is_original ? table.original : table.add;
        const std::string source = is_original ? "Original" : "Add";
        const std::string piece_str = buffer.substr(piece.start, piece.length);

        out << std::format("Piece(start={}, length={}, source={}): \"{}\"\n", piece.start,
                           piece.length, source, EscapeSpecialChars(piece_str));
    }
    return out;
}

PieceTable::Iterator PieceTable::begin() {
    return {*this, pieces.begin(), 0};
}

PieceTable::Iterator PieceTable::end() {
    return {*this, pieces.end(), 0};
}

PieceTable::Iterator PieceTable::line(size_t line_index) {
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
    if (piece_index == piece_it->length) {
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

}
