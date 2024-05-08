#include "piece_table.h"
#include <iostream>

PieceTable::PieceTable(std::string_view str) : original(str) {
    pieces.emplace_back(PieceDescriptor{
        .start = 0,
        .length = str.length(),
        .source = PieceSource::Original,
    });
}

void PieceTable::insert(size_t start, std::string_view str) {
    size_t add_start = add.size();
    add += str;

    for (size_t i = 0; i < pieces.size(); i++) {
        auto& piece = pieces[i];

        if (start < piece.start + piece.length) {
            // Split piece into three pieces.
            size_t old_length = piece.length;
            piece.length = start;

            pieces.insert(pieces.begin() + i + 1, {
                                                      .start = add_start,
                                                      .length = add_start + str.length(),
                                                      .source = PieceSource::Add,
                                                  });

            pieces.insert(pieces.begin() + i + 2, {
                                                      .start = start,
                                                      .length = old_length - start,
                                                      .source = PieceSource::Original,
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

static inline std::string EscapeSpecialChars(const std::string& str) {
    std::string result;

    for (auto c : str) {
        switch (c) {
        case '\n':
            result += "\\n";
            break;

        case '\t':
            result += "\\t";
            break;

        default:
            result += c;
            break;
        }
    }

    return result;
}

void PieceTable::printPieces() {
    std::cerr << "original: \"" << EscapeSpecialChars(original) << "\"\n";
    std::cerr << "add:      \"" << EscapeSpecialChars(add) << "\"\n";

    for (const auto& piece : pieces) {
        const char* source = piece.source == PieceSource::Original ? "original" : "add";
        fprintf(stderr, "Piece(start=%zu, length=%zu, source=%s)\n", piece.start, piece.length,
                source);
    }
}
