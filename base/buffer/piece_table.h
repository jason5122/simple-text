#pragma once

#include <cstddef>
#include <iterator>
#include <list>
#include <string>

namespace base {

class PieceTable {
private:
    // We need to forward declare `Piece` since `Iterator` references it.
    struct Piece;
    using PieceIterator = std::list<Piece>::iterator;

public:
    PieceTable(std::string_view str);

    void insert(size_t index, std::string_view str);
    void erase(size_t index, size_t count);
    size_t length() const;
    size_t newlineCount() const;
    std::string line(size_t index) const;
    std::string str() const;

    friend std::ostream& operator<<(std::ostream& out, const PieceTable& table);

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = char;
        using pointer = char*;
        using reference = char&;

        reference operator*() const;
        pointer operator->();
        Iterator& operator++();
        Iterator operator++(int);

        friend bool operator==(const Iterator& a, const Iterator& b);
        friend bool operator!=(const Iterator& a, const Iterator& b);

        friend std::ostream& operator<<(std::ostream& out, const Iterator& it);

    private:
        friend class PieceTable;

        Iterator(PieceTable& table, PieceIterator piece_it, size_t piece_index)
            : table{table}, piece_it{piece_it}, piece_index{piece_index} {}

        PieceTable& table;
        PieceIterator piece_it;
        size_t piece_index;

        // TODO: This is needed for `operator<<()`. Can we work around this?
        std::list<Piece>& pieces() const {
            return table.pieces;
        }
    };

    Iterator begin();
    Iterator end();
    Iterator newline(size_t index);  // Zero-indexed.

private:
    enum class PieceSource {
        Original,
        Add,
    };

    struct Piece {
        PieceSource source;
        size_t start;
        size_t length;
        std::list<size_t> newlines;
    };

    std::string original;
    std::string add;
    std::list<Piece> pieces;
    size_t m_length = 0;
    size_t newline_count = 0;

    std::pair<PieceIterator, size_t> pieceAt(size_t index);
    std::list<size_t> cacheNewlines(std::string_view str);
};

}
