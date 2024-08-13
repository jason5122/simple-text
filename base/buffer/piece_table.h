#pragma once

#include <cstddef>
#include <iterator>
#include <list>
#include <string>

namespace base {

class PieceTable {
public:
    PieceTable(std::string_view str);

    void insert(size_t index, std::string_view str);
    void erase(size_t index, size_t count);
    void insert(size_t line, size_t column, std::string_view str);
    void erase(size_t line, size_t column, size_t count);
    void erase(size_t start_line, size_t start_column, size_t end_line, size_t end_column);
    size_t length() const;
    size_t lineCount() const;
    size_t newlineCount() const;
    std::string line(size_t index);  // Zero-indexed.
    std::string str() const;

    struct Iterator;
    struct ConstIterator;
    using iterator = Iterator;
    using const_iterator = ConstIterator;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;

    iterator newline(size_t index);              // Zero-indexed.
    const_iterator newline(size_t index) const;  // Zero-indexed.

    friend std::ostream& operator<<(std::ostream& out, const PieceTable& table);
    // This is needed for `ConstIterator` to access `pieces`.
    friend std::ostream& operator<<(std::ostream& out, const ConstIterator& it);
    // This is needed for `Iterator` to access `pieces`.
    friend std::ostream& operator<<(std::ostream& out, const Iterator& it);

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

    std::unordered_map<size_t, std::string> line_cache;

    using PieceIterator = std::list<Piece>::iterator;
    using PieceConstIterator = std::list<Piece>::const_iterator;
    std::pair<PieceIterator, size_t> pieceAt(size_t index);
    std::list<size_t> cacheNewlines(std::string_view str);
};

struct PieceTable::ConstIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const char;
    using pointer = const char*;
    using reference = const char&;

    reference operator*() const;
    pointer operator->();
    ConstIterator& operator++();
    ConstIterator operator++(int);

    friend bool operator==(const ConstIterator& a, const ConstIterator& b);
    friend bool operator!=(const ConstIterator& a, const ConstIterator& b);

    friend std::ostream& operator<<(std::ostream& out, const ConstIterator& it);
    friend std::ostream& operator<<(std::ostream& out, const Iterator& it);

private:
    friend class PieceTable;

    ConstIterator(const PieceTable& table, PieceConstIterator piece_it, size_t piece_index);

    const PieceTable& table;
    PieceConstIterator piece_it;
    size_t piece_index;
};

struct PieceTable::Iterator : public ConstIterator {
    using reference = char&;

    reference operator*() const;

    friend std::ostream& operator<<(std::ostream& out, const Iterator& it);

private:
    friend class PieceTable;

    Iterator(PieceTable& table, PieceIterator piece_it, size_t piece_index);

    PieceTable& table;
};

static_assert(std::is_trivially_copy_constructible_v<PieceTable::ConstIterator>);
static_assert(std::is_trivially_copy_constructible_v<PieceTable::Iterator>);

}
