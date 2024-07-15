#include "piece_table.h"
#include "util/escape_special_chars.h"
#include <format>
#include <iostream>

namespace base {

PieceTable::PieceTable(std::string_view str) : original(str) {
    pieces.emplace_front(Piece{
        .source = PieceSource::Original,
        .start = 0,
        .length = str.length(),
    });
}

void PieceTable::insert(size_t start, std::string_view str) {
    size_t add_start = add.size();
    add += str;

    for (auto it = pieces.begin(); it != pieces.end(); it++) {
        // Split piece into three pieces.
        auto& piece = *it;
        if (start < piece.start + piece.length) {
            size_t old_length = piece.length;
            piece.length = start;

            pieces.emplace_after(it, Piece{
                                         .source = PieceSource::Add,
                                         .start = add_start,
                                         .length = add_start + str.length(),
                                     });
            pieces.emplace_after(std::next(it), Piece{
                                                    .source = PieceSource::Original,
                                                    .start = start,
                                                    .length = old_length - start,
                                                });
            break;
        }
    }
}

void PieceTable::erase(size_t pos, size_t count) {
    for (auto it = pieces.begin(); it != pieces.end(); it++) {
        // Split piece into two pieces.
        auto& piece = *it;
        if (pos >= piece.start && pos + count < piece.start + piece.length) {
            size_t old_length = piece.length;
            piece.length = pos - piece.start;

            pieces.emplace_after(it, Piece{
                                         .source = piece.source,
                                         .start = pos + count,
                                         .length = old_length - count,
                                     });
            break;
        }
    }
}

std::string PieceTable::string() {
    std::string str;
    for (const auto& piece : pieces) {
        std::string& buffer = piece.source == PieceSource::Original ? original : add;
        str += buffer.substr(piece.start, piece.length);
    }
    return str;
}

void PieceTable::printPieces() {
    std::cerr << "original: \"" << EscapeSpecialChars(original) << "\"\n";
    std::cerr << "add:      \"" << EscapeSpecialChars(add) << "\"\n";

    for (const auto& piece : pieces) {
        const char* source = piece.source == PieceSource::Original ? "original" : "add";
        std::cerr << std::format("Piece(start={}, length={}, source={})\n", piece.start,
                                 piece.length, source);
    }
}

}
