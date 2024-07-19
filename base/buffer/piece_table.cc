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
        std::cerr << std::format("Current piece: Piece(start={}, length={})\n", (*it).start,
                                 (*it).length);
        piece_start += (*it).length;
        piece_end = piece_start + (*it).length;
        it++;
    }

    if (it == pieces.end()) {
        std::cerr << "PieceTable::insert() error: reached end, returning\n";
        return;
    }

    Piece& p1 = *it;
    size_t add_start = add.length();
    add += str;

    // Case 1: Beginning of a piece. We only need to add a single piece.
    if (start == piece_start) {
        std::cerr << "Case 1\n";

        const Piece p0{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };
        pieces.insert(it, p0);
    }
    // Case 2: End of a piece. We only need to add a single piece.
    else if (start == piece_end) {
        std::cerr << "Case 2\n";

        const Piece p0{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };
        pieces.insert(std::next(it), p0);
    }
    // Case 3: Middle of a piece. We need to split one piece into three pieces.
    else {
        std::cerr << "Case 3\n";
        std::cerr << std::format("Chosen piece: Piece(start={}, length={})\n", p1.start,
                                 p1.length);

        size_t old_length = p1.length;
        p1.length = start - piece_start;

        const Piece p2{
            .source = PieceSource::Add,
            .start = add_start,
            .length = str.length(),
        };

        std::cerr << std::format("[{}, {}]\n", piece_start, piece_end);
        if (old_length < start) {
            std::cerr << std::format("{} < {}\n", old_length, start);
        }
        const Piece p3{
            .source = PieceSource::Original,
            .start = p1.start + p1.length,
            // .length = old_length - start,
            .length = piece_end - start,
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
