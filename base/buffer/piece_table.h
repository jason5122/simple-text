#pragma once

#include <forward_list>
#include <string>
#include <string_view>

namespace base {

// https://darrenburns.net/posts/piece-table/
class PieceTable {
public:
    PieceTable(std::string_view str);

    void insert(size_t start, std::string_view str);
    void erase(size_t pos, size_t count);
    std::string string();

    // DEBUG: Visualizes pieces.
    void printPieces();

private:
    enum class PieceSource {
        Original,
        Add,
    };

    struct Piece {
        PieceSource source;
        size_t start;
        size_t length;
        std::forward_list<size_t> line_starts;
    };

    std::string original;
    std::string add;
    std::forward_list<Piece> pieces;
};

}
