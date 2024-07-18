#pragma once

#include <forward_list>
#include <string>

namespace base {

class PieceTable {
public:
    PieceTable(std::string_view str);

    void insert(size_t start, std::string_view str);
    void erase(size_t pos, size_t count);
    std::string str();

    friend std::ostream& operator<<(std::ostream& out, const PieceTable& table);

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
