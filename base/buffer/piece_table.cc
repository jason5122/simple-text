#include "piece_table.h"
#include "util/escape_special_chars.h"
#include <format>
#include <iostream>

namespace base {

PieceTable::PieceTable(std::string_view str) : original{str}, _length{str.length()} {
    pieces.emplace_front(Piece{
        .source = PieceSource::Original,
        .start = 0,
        .length = str.length(),
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
        std::cerr << "Case 1\n";
        size_t p1_old_length = p1.length;
        p1.length = index - piece_start;

        const Piece p2{
            .source = p1.source,
            .start = p1.start + p1.length + count,
            .length = p1_old_length - count,
        };
        pieces.insert(std::next(it), p2);
        return;
    }

    // Case 2: Erase spans multiple pieces.
    std::cerr << "Case 2\n";
    auto start_it = it;  // TODO: See if we can do this in a cleaner way.
    while (it != pieces.end() && count > 0) {
        Piece& piece = *it;
        size_t piece_start = offset;
        size_t piece_end = offset + (*it).length;

        // TODO: See if we can do this in a cleaner way.
        bool temp = true;
        if (it == start_it) {
            piece_start = index;
            temp = false;
            std::cerr << "...no...\n";
        }

        std::cerr << std::format("{{{}, {}}}\n", piece_end - piece_start, count);
        size_t sub = std::min(piece_end - piece_start, count);
        if (count >= sub) {
            std::cerr << std::format("Updating piece length from {} to {}\n", piece.length,
                                     piece.length - sub);
            if (temp) piece.start += sub;
            piece.length -= sub;
            count -= sub;
        }

        offset += (*it).length;
        it++;
    }
}

size_t PieceTable::length() {
    return _length;
}

std::string PieceTable::str() {
    std::string str;
    for (const auto& piece : pieces) {
        const std::string& buffer = piece.source == PieceSource::Original ? original : add;
        str += buffer.substr(piece.start, piece.length);
    }
    return str;
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

}
