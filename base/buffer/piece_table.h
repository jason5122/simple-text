#pragma once

#include <cstddef>
#include <iterator>
#include <list>
#include <string>

namespace base {

class PieceTable {
public:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        using pointer = char*;
        using reference = char&;

        Iterator(pointer ptr) : _ptr{ptr} {}

        reference operator*() const;
        pointer operator->();
        Iterator& operator++();
        Iterator operator++(int);

        friend bool operator==(const Iterator& a, const Iterator& b);
        friend bool operator!=(const Iterator& a, const Iterator& b);

    private:
        pointer _ptr;
    };

    PieceTable(std::string_view str);

    Iterator begin();
    Iterator end();

    void insert(size_t index, std::string_view str);
    void erase(size_t index, size_t count);
    size_t length();
    size_t lineCount();
    std::string str();
    std::string line(size_t line_index);  // Zero-indexed.

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
    size_t _line_count = 0;
};

}
