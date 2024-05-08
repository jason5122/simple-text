#pragma once

#include <string>
#include <string_view>
#include <vector>

// https://darrenburns.net/posts/piece-table/
class PieceTable {
public:
    PieceTable(std::string_view str);

    void insert(size_t start, std::string_view str);
    std::string string();

    // DEBUG: Visualizes pieces.
    void printPieces();

private:
    enum class PieceSource {
        Original,
        Add,
    };

    struct PieceDescriptor {
        size_t start;
        size_t length;
        PieceSource source;
    };

    std::string original;
    std::string add;
    std::vector<PieceDescriptor> pieces;
};
