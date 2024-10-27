#pragma once

#include <forward_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "fredbuf_rbtree.h"

#ifndef NDEBUG
#define TEXTBUF_DEBUG
#endif  // NDEBUG

namespace PieceTree {

constexpr auto kSentinel = std::numeric_limits<size_t>::max();

struct UndoRedoEntry {
    RedBlackTree root;
    size_t op_offset;
};

using LineStarts = std::vector<size_t>;

struct NodePosition {
    // Piece Index
    const PieceTree::NodeData* node = nullptr;
    // Remainder in current piece.
    size_t remainder = 0;
    // Node start offset in document.
    size_t start_offset = 0;
    // The line (relative to the document) where this node starts.
    size_t line = 0;
};

struct CharBuffer {
    std::string buffer;
    LineStarts line_starts;
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

class Tree {
public:
    explicit Tree();
    explicit Tree(std::string_view txt);

    // Manipulation.
    void insert(size_t offset, std::string_view txt);
    void erase(size_t offset, size_t count);
    UndoRedoResult try_undo(size_t op_offset = 0);
    UndoRedoResult try_redo(size_t op_offset = 0);

    // Queries.
    std::string get_line_content(size_t line) const;
    char at(size_t offset) const;
    size_t line_at(size_t offset) const;
    std::pair<size_t, size_t> line_column_at(size_t offset) const;
    size_t offset_at(size_t line, size_t column) const;
    size_t get_newline_offset(size_t line) const;  // TODO: Consider removing this method.
    LineRange get_line_range(size_t line) const;
    LineRange get_line_range_with_newline(size_t line) const;
    std::string str() const;
    std::string substr(size_t offset, size_t count) const;

    size_t length() const;
    bool empty() const;
    size_t line_feed_count() const;
    size_t line_count() const;

    // TODO: Make this private again after we're done debugging.
    // private:
    friend class TreeWalker;
    friend class ReverseTreeWalker;

    // Initialization after populating initial immutable buffers from ctor.
    void build_tree();

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
    void assemble_line(std::string* buf, const RedBlackTree& node, size_t line) const;
    Piece build_piece(std::string_view txt);
    void combine_pieces(NodePosition existing_piece, Piece new_piece);
    void remove_node_range(NodePosition first, size_t length);
    void compute_buffer_meta();
    void append_undo(const RedBlackTree& old_root, size_t op_offset);

public:  // TODO: Remove this public block.
    BufferCollection buffers;
    PieceTree::RedBlackTree root;
    BufferCursor last_insert;
    // Note: This is absolute position.  Initialize to nonsense value.
    size_t end_last_insert = kSentinel;

    // Buffer metadata.
    size_t lf_count = 0;
    size_t total_content_length = 0;

    std::forward_list<UndoRedoEntry> undo_stack;
    std::forward_list<UndoRedoEntry> redo_stack;
};

class TreeWalker {
public:
    TreeWalker(const Tree* tree, size_t offset = 0);
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
        PieceTree::RedBlackTree node;
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
    ReverseTreeWalker(const Tree* tree, size_t offset = 0);
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
        PieceTree::RedBlackTree node;
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

inline TreeWalker begin(const Tree& tree) {
    return TreeWalker{&tree};
}

constexpr WalkSentinel end(const Tree&) {
    return WalkSentinel{};
}

inline bool operator==(const TreeWalker& walker, WalkSentinel) {
    return walker.exhausted();
}

// TODO: Debug use; remove this.
void print_piece(const Piece& piece, const Tree* tree, int level);
void print_tree(const PieceTree::RedBlackTree& root,
                const PieceTree::Tree* tree,
                int level = 0,
                size_t node_offset = 0);

}  // namespace PieceTree
