#pragma once

#include "base/buffer/piece_tree_rbtree.h"
#include <forward_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifndef NDEBUG
#define TEXTBUF_DEBUG
#endif  // NDEBUG

namespace base {

struct NodePosition {
    const NodeData* node = nullptr;
    size_t remainder = 0;     // Remainder in current piece.
    size_t start_offset = 0;  // Node start offset in document.
    size_t line = 0;          // The line (relative to the document) where this node starts.
};

struct CharBuffer {
    std::string buffer;
    std::vector<size_t> line_starts;
};

struct BufferCollection {
    const CharBuffer* buffer_at(BufferType buffer_type) const;
    size_t buffer_offset(BufferType buffer_type, const BufferCursor& cursor) const;

    std::shared_ptr<const CharBuffer> orig_buffer;
    CharBuffer mod_buffer;
};

struct LineRange {
    size_t first;
    size_t last;
};

class PieceTree {
public:
    explicit PieceTree();
    explicit PieceTree(std::string_view txt);

    // Manipulation.
    void insert(size_t offset, std::string_view txt);
    void erase(size_t offset, size_t count);
    bool undo();
    bool redo();

    // Queries.
    std::string get_line_content(size_t line) const;
    std::string get_line_content_with_newline(size_t line) const;
    // This is similar to `get_line_content_with_newline`, except newlines are replaced by spaces.
    std::string get_line_content_for_layout_use(size_t line) const;
    size_t line_at(size_t offset) const;
    BufferCursor line_column_at(size_t offset) const;
    size_t offset_at(size_t line, size_t column) const;
    LineRange get_line_range(size_t line) const;
    LineRange get_line_range_with_newline(size_t line) const;
    std::string str() const;
    std::string substr(size_t offset, size_t count) const;
    std::optional<size_t> find(std::string_view str) const;

    size_t length() const;
    bool empty() const;
    size_t line_feed_count() const;
    size_t line_count() const;

private:
    friend class TreeWalker;
    friend class ReverseTreeWalker;

    void internal_insert(size_t offset, std::string_view txt);
    void internal_erase(size_t offset, size_t count);

    using Accumulator = size_t (*)(const BufferCollection*, const Piece&, size_t);

    template <Accumulator accumulate>
    static void line_start(size_t* offset,
                           const BufferCollection* buffers,
                           const RedBlackTree& node,
                           size_t line);
    static size_t accumulate_value(const BufferCollection* buffers,
                                   const Piece& piece,
                                   size_t index);
    static size_t accumulate_value_no_lf(const BufferCollection* buffers,
                                         const Piece& piece,
                                         size_t index);
    size_t line_feed_count(BufferType buffer_type,
                           const BufferCursor& start,
                           const BufferCursor& end) const;
    NodePosition node_at(size_t off) const;
    BufferCursor buffer_position(const Piece& piece, size_t remainder) const;
    Piece trim_piece_right(const Piece& piece, const BufferCursor& pos) const;
    Piece trim_piece_left(const Piece& piece, const BufferCursor& pos) const;

    struct ShrinkResult {
        Piece left;
        Piece right;
    };

    ShrinkResult shrink_piece(const Piece& piece,
                              const BufferCursor& first,
                              const BufferCursor& last) const;

    // Direct mutations.
    Piece build_piece(std::string_view txt);
    void combine_pieces(NodePosition existing_piece, Piece new_piece);
    void remove_node_range(NodePosition first, size_t length);
    void compute_buffer_meta();
    void append_undo();

    BufferCollection buffers;
    RedBlackTree root;
    BufferCursor last_insert;

    // Buffer metadata.
    size_t lf_count = 0;
    size_t total_content_length = 0;

    std::forward_list<RedBlackTree> undo_stack;
    std::forward_list<RedBlackTree> redo_stack;
};

class TreeWalker {
public:
    TreeWalker(const PieceTree* tree, size_t offset = 0);

    char current();
    char next();
    char32_t next_codepoint();
    void seek(size_t offset);
    bool exhausted() const;
    constexpr size_t remaining() const;
    constexpr size_t offset() const;

private:
    void populate_ptrs();
    void fast_forward_to(size_t offset);

    enum class Direction { Left, Center, Right };

    struct StackEntry {
        RedBlackTree node;
        Direction dir = Direction::Left;
    };

    const BufferCollection* buffers;
    RedBlackTree root;

    // Buffer metadata.
    size_t total_content_length = 0;

    std::vector<StackEntry> stack;
    size_t total_offset = 0;
    const char* first_ptr = nullptr;
    const char* last_ptr = nullptr;
};

class ReverseTreeWalker {
public:
    ReverseTreeWalker(const PieceTree* tree, size_t offset = 0);

    char current();
    char next();
    char32_t next_codepoint();
    void seek(size_t offset);
    bool exhausted() const;
    constexpr size_t remaining() const;
    constexpr size_t offset() const;

private:
    void populate_ptrs();
    void fast_forward_to(size_t offset);

    enum class Direction { Left, Center, Right };

    struct StackEntry {
        RedBlackTree node;
        Direction dir = Direction::Right;
    };

    const BufferCollection* buffers;
    RedBlackTree root;
    std::vector<StackEntry> stack;
    size_t total_offset = 0;
    const char* first_ptr = nullptr;
    const char* last_ptr = nullptr;
};

constexpr size_t TreeWalker::remaining() const { return total_content_length - total_offset; }

constexpr size_t TreeWalker::offset() const { return total_offset; }

constexpr size_t ReverseTreeWalker::remaining() const { return total_offset + 1; }

constexpr size_t ReverseTreeWalker::offset() const { return total_offset; }

}  // namespace base
