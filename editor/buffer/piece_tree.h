#pragma once

#include "editor/buffer/red_black_tree.h"
#include <format>
#include <forward_list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace editor {

struct NodePosition;

struct CharBuffer {
    std::string buffer;
    std::vector<size_t> line_starts;
};

struct BufferCollection {
    CharBuffer orig_buffer;
    CharBuffer mod_buffer;
};

struct LineRange {
    size_t first{};
    size_t last{};
};

class PieceTree {
public:
    PieceTree() : PieceTree(std::string_view{}) {}
    explicit PieceTree(std::string_view txt);
    PieceTree& operator=(std::string_view txt);

    // Manipulation.
    void insert(size_t offset, std::string_view txt);
    void erase(size_t offset, size_t count);
    void clear();
    bool undo();
    bool redo();

    // Metadata.
    size_t length() const { return root_.length(); }
    bool empty() const { return length() == 0; }
    size_t line_feed_count() const { return root_.line_feed_count(); }
    size_t line_count() const { return line_feed_count() + 1; }

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

private:
    friend class TreeWalker;
    friend class ReverseTreeWalker;

    // Direct mutations.
    Piece build_piece(std::string_view txt);
    void combine_pieces(NodePosition existing_piece, Piece new_piece);
    void remove_node_range(NodePosition first, size_t length);

    BufferCollection buffers_;
    RedBlackTree root_;
    BufferCursor last_insert_;

    std::forward_list<RedBlackTree> undo_stack_;
    std::forward_list<RedBlackTree> redo_stack_;
};

class TreeWalker {
public:
    TreeWalker(const PieceTree& tree, size_t offset = 0);

    char current();
    char next();
    char32_t next_codepoint();
    void seek(size_t offset);
    bool exhausted() const;
    constexpr size_t remaining() const { return length_ - total_offset_; }
    constexpr size_t offset() const { return total_offset_; }

private:
    void populate_ptrs();
    void fast_forward_to(size_t offset);

    enum class Direction { Left, Center, Right };

    struct StackEntry {
        RedBlackTree node;
        Direction dir = Direction::Left;
    };

    const BufferCollection& buffers_;
    RedBlackTree root_;
    size_t length_ = 0;

    std::vector<StackEntry> stack_;
    size_t total_offset_ = 0;
    const char* first_ptr_ = nullptr;
    const char* last_ptr_ = nullptr;
};

class ReverseTreeWalker {
public:
    ReverseTreeWalker(const PieceTree& tree, size_t offset = 0);

    char current();
    char next();
    char32_t next_codepoint();
    void seek(size_t offset);
    bool exhausted() const;
    constexpr size_t remaining() const { return total_offset_ + 1; }
    constexpr size_t offset() const { return total_offset_; }

private:
    void populate_ptrs();
    void fast_forward_to(size_t offset);

    enum class Direction { Left, Center, Right };

    struct StackEntry {
        RedBlackTree node;
        Direction dir = Direction::Right;
    };

    const BufferCollection& buffers_;
    RedBlackTree root_;
    std::vector<StackEntry> stack_;
    size_t total_offset_ = 0;
    const char* first_ptr_ = nullptr;
    const char* last_ptr_ = nullptr;
};

}  // namespace editor

template <>
struct std::formatter<editor::PieceTree> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <class FormatContext>
    auto format(const editor::PieceTree& pt, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", pt.str());
    }
};

template <>
struct std::formatter<editor::LineRange> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <class FormatContext>
    auto format(const editor::LineRange& r, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "[{}..{}]", r.first, r.last);
    }
};
