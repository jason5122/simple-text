#include "piece_table.h"
#include "util/escape_special_chars.h"
#include <format>
#include <iostream>

namespace base {

PieceTable::PieceTable(std::string_view str) : original{str} {
    pieces.emplace_front(Piece{
        .source = PieceSource::Original,
        .start = 0,
        .length = str.length(),
    });
}

void PieceTable::insert(size_t start, std::string_view str) {
    auto it = pieces.begin();
    size_t piece_start = 0;
    size_t piece_end = (*it).length;
    while (it != pieces.end() && start > piece_end) {
        piece_start += (*it).length;
        piece_end = piece_start + (*it).length;
        it++;
    }

    if (it == pieces.end()) {
        std::cerr << "PieceTable::insert() error: reached end, returning\n";
        return;
    }

    // Append string to `add` buffer.
    size_t add_start = add.length();
    add += str;

    Piece& p1 = *it;

    // Case 1: Beginning/end of a piece. We only need to add a single piece.
    if (start == piece_start || start == piece_end) {
        const Piece p{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };
        auto pos = start == piece_start ? it : std::next(it);
        pieces.insert(pos, p);
    }
    // Case 2: Middle of a piece. We need to split one piece into three pieces.
    else {
        size_t p1_old_length = p1.length;
        p1.length = start - piece_start;

        const Piece p2{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };
        const Piece p3{
            .source = PieceSource::Original,
            .start = p1.start + p1.length,
            .length = p1_old_length - p1.length,
        };
        auto it2 = pieces.insert(std::next(it), p2);
        pieces.insert(std::next(it2), p3);
    }
}

void PieceTable::erase(size_t pos, size_t count) {
    for (auto it = pieces.begin(); it != pieces.end(); it++) {
        // Split piece into two pieces.
        auto& p1 = *it;
        if (pos >= p1.start && pos + count < p1.start + p1.length) {
            size_t old_length = p1.length;
            p1.length = pos - p1.start;

            const Piece p2{
                .source = p1.source,
                .start = pos + count,
                .length = old_length - count,
            };

            pieces.insert(it, p2);
            break;
        }
    }
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
        const std::string piece_str = buffer.substr(piece.start, piece.length);
        const std::string source = is_original ? "Original" : "Add";

        out << std::format("Piece(start={}, length={}, source={}): \"{}\"\n", piece.start,
                           piece.length, source, EscapeSpecialChars(piece_str));
    }
    return out;
}

}
