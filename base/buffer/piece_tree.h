#pragma once

#include <forward_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "piece_tree_rbtree.h"

#ifndef NDEBUG
#define TEXTBUF_DEBUG
#endif  // NDEBUG

namespace base {

struct UndoRedoEntry {
    RedBlackTree root;
    size_t op_offset;
};

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

struct UndoRedoResult {
    bool success;
    size_t op_offset;
};

class PieceTree {
public:
    explicit PieceTree();
    explicit PieceTree(std::string_view txt);

    // Manipulation.
    void insert(size_t offset, std::string_view txt);
    void erase(size_t offset, size_t count);
    UndoRedoResult try_undo(size_t op_offset = 0);
    UndoRedoResult try_redo(size_t op_offset = 0);

    // Queries.
    std::string get_line_content(size_t line) const;
    std::string get_line_content_with_newline(size_t line) const;
    // This is similar to `get_line_content_with_newline`, except newlines are replaced by spaces.
    std::string get_line_content_for_layout_use(size_t line) const;
    char at(size_t offset) const;
    size_t line_at(size_t offset) const;
    BufferCursor line_column_at(size_t offset) const;
    size_t offset_at(size_t line, size_t column) const;
    LineRange get_line_range(size_t line) const;
    LineRange get_line_range_with_newline(size_t line) const;
    std::string str() const;
    std::string substr(size_t offset, size_t count) const;

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
    void append_undo(const RedBlackTree& old_root, size_t op_offset);

    BufferCollection buffers;
    RedBlackTree root;
    BufferCursor last_insert;

    // Buffer metadata.
    size_t lf_count = 0;
    size_t total_content_length = 0;

    std::forward_list<UndoRedoEntry> undo_stack;
    std::forward_list<UndoRedoEntry> redo_stack;
};

class TreeWalker {
public:
    TreeWalker(const PieceTree* tree, size_t offset = 0);
    TreeWalker(const TreeWalker&) = delete;

    char current();
    char next();
    void seek(size_t offset);
    bool exhausted() const;
    size_t remaining() const;
    size_t offset() const {
        return total_offset;
    }

    // For Iterator-like behavior.
    TreeWalker& operator++() {
        return *this;
    }

    char operator*() {
        return next();
    }

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
    ReverseTreeWalker(const TreeWalker&) = delete;

    char current();
    char next();
    void seek(size_t offset);
    bool exhausted() const;
    size_t remaining() const;
    size_t offset() const {
        return total_offset;
    }

    // For Iterator-like behavior.
    ReverseTreeWalker& operator++() {
        return *this;
    }

    char operator*() {
        return next();
    }

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

struct WalkSentinel {};

inline TreeWalker begin(const PieceTree& tree) {
    return TreeWalker{&tree};
}

constexpr WalkSentinel end(const PieceTree&) {
    return WalkSentinel{};
}

inline bool operator==(const TreeWalker& walker, WalkSentinel) {
    return walker.exhausted();
}

}  // namespace base
