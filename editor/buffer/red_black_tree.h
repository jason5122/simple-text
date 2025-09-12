#pragma once

#include "base/check.h"
#include <format>
#include <memory>

namespace editor {

struct BufferCursor {
    size_t line{};    // Relative line in the current buffer.
    size_t column{};  // Column into the current line.

    bool operator==(const BufferCursor&) const = default;
};

enum class BufferType { Original, Mod };

struct Piece {
    BufferType type{};
    BufferCursor first{};
    BufferCursor last{};
    size_t length{};
    size_t lf_count{};
};

struct NodeData {
    Piece piece{};
    size_t left_length{};
    size_t left_lf_count{};
    size_t subtree_length{};
    size_t subtree_lf_count{};
};

enum class Color { Red, Black };

class RedBlackTree {
    struct Node;
    using NodePtr = std::shared_ptr<const Node>;

    struct Node {
        Color color;
        NodePtr left;
        NodeData data;
        NodePtr right;
    };

public:
    RedBlackTree() = default;
    RedBlackTree(Color c, const RedBlackTree& lft, const Piece& p, const RedBlackTree& rgt);

    // Queries.
    explicit operator bool() const { return static_cast<bool>(node_); }
    size_t length() const { return !node_ ? 0 : node_->data.subtree_length; }
    size_t line_feed_count() const { return !node_ ? 0 : node_->data.subtree_lf_count; }
    size_t left_length() const { return !node_ ? 0 : node_->data.left_length; }
    size_t left_line_feed_count() const { return !node_ ? 0 : node_->data.left_lf_count; }
    // clang-format off
    const Piece& piece() const { DCHECK(node_); return node_->data.piece; }
    Color color() const { DCHECK(node_); return node_->color; }
    RedBlackTree left() const { DCHECK(node_); return {node_->left}; }
    RedBlackTree right() const { DCHECK(node_); return {node_->right}; }
    // clang-format on

    // Mutators.
    RedBlackTree insert(size_t at, const Piece& p) const;
    RedBlackTree remove(size_t at) const;

    // Helpers.
    bool operator==(const RedBlackTree&) const = default;

    // Debug use.
    std::string to_string() const;
    bool check_invariants() const;

private:
    RedBlackTree(const NodePtr& node) : node_(node) {}

    NodePtr node_;
};

}  // namespace editor

template <>
struct std::formatter<editor::RedBlackTree> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    template <class FormatContext>
    auto format(const editor::RedBlackTree& t, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "{}", t.to_string());
    }
};
