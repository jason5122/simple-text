#pragma once

#include <list>
#include <string>

namespace base {

class PieceTable {
public:
    PieceTable(std::string_view str);

    void insert(size_t index, std::string_view str);
    void erase(size_t index, size_t count);
    size_t length();
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
        std::list<size_t> line_starts;
    };

    std::string original;
    std::string add;
    std::list<Piece> pieces;
    size_t _length = 0;
};

}