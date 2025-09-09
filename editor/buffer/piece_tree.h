#pragma once

#include "editor/buffer/piece_tree_rbtree.h"
#include <forward_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace editor {

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
    size_t first{};
    size_t last{};
};

class PieceTree {
public:
    PieceTree();
    PieceTree(std::string_view txt);
    PieceTree(const char* s);
    PieceTree(const std::string& s);

    PieceTree& operator=(std::string_view txt);
    PieceTree& operator=(const char* s);
    PieceTree& operator=(const std::string& s);

    // Manipulation.
    void insert(size_t offset, std::string_view txt);
    void erase(size_t offset, size_t count);
    void clear();
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

    size_t size() const;
    size_t length() const;
    bool empty() const;
    size_t line_feed_count() const;
    size_t line_count() const;

private:
    friend class TreeWalker;
    friend class ReverseTreeWalker;

    // Direct mutations.
    void assign(std::string_view txt);
    Piece build_piece(std::string_view txt);
    void combine_pieces(NodePosition existing_piece, Piece new_piece);
    void remove_node_range(NodePosition first, size_t length);

    BufferCollection buffers_;
    RedBlackTree root_;
    BufferCursor last_insert_;

    // Buffer metadata.
    size_t lf_count_ = 0;
    size_t total_content_length_ = 0;

    std::forward_list<RedBlackTree> undo_stack_;
    std::forward_list<RedBlackTree> redo_stack_;
};

class TreeWalker {
public:
    TreeWalker(const PieceTree* tree, size_t offset = 0);

    char current();
    char next();
    char32_t next_codepoint();
    void seek(size_t offset);
    bool exhausted() const;
    constexpr size_t remaining() const { return total_content_length - total_offset; }
    constexpr size_t offset() const { return total_offset; }

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
    constexpr size_t remaining() const { return total_offset + 1; }
    constexpr size_t offset() const { return total_offset; }

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

}  // namespace editor
