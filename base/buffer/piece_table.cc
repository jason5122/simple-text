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
    size_t add_start = add.length();
    add += str;

    for (auto it = pieces.begin(); it != pieces.end(); it++) {
        // Split piece into three pieces.
        auto& p1 = *it;
        if (start < p1.start + p1.length) {
            size_t old_length = p1.length;
            p1.length = start;

            const Piece p2{
                .source = PieceSource::Add,
                .start = add_start,
                .length = add_start + str.length(),
            };
            const Piece p3{
                .source = PieceSource::Original,
                .start = start,
                .length = old_length - start,
            };

            auto it2 = pieces.emplace_after(it, p2);
            pieces.emplace_after(it2, p3);
            break;
        }
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

            pieces.emplace_after(it, p2);
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
        const std::string source =
            piece.source == PieceTable::PieceSource::Original ? "original" : "add";
        out << std::format("Piece(start={}, length={}, source={})\n", piece.start, piece.length,
                           source);
    }
    return out;
}

}
