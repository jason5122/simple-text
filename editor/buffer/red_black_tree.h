#pragma once

#include "base/check.h"
#include <memory>

namespace editor {

struct BufferCursor {
    size_t line{};    // Relative line in the current buffer.
    size_t column{};  // Column into the current line.

    bool operator==(const BufferCursor&) const = default;
};

enum class BufferType { Original, Mod };

struct Piece {
    BufferType buffer_type{};
    BufferCursor first{};
    BufferCursor last{};
    size_t length{};
    size_t newline_count{};
};

struct NodeData {
    Piece piece{};
    size_t left_subtree_length{};
    size_t left_subtree_lf_count{};
    size_t subtree_length{};
    size_t subtree_lf_count{};
};

enum class Color { Red, Black };

class RedBlackTree {
    struct Node;
    using NodePtr = std::shared_ptr<const Node>;

    struct Node {
        Node(Color c, const NodePtr& lft, const NodeData& data, const NodePtr& rgt);

        Color color;
        NodePtr left;
        NodeData data;
        NodePtr right;
    };

public:
    RedBlackTree() = default;
    RedBlackTree(Color c, const RedBlackTree& lft, const NodeData& val, const RedBlackTree& rgt);
    RedBlackTree(const NodePtr& node);

    // Queries.
    explicit operator bool() const noexcept { return static_cast<bool>(node_); }
    constexpr size_t length() const { return !node_ ? 0 : data().subtree_length; }
    constexpr size_t line_feed_count() const { return !node_ ? 0 : data().subtree_lf_count; }
    // clang-format off
    const NodeData& data() const { DCHECK(node_); return node_->data; }
    RedBlackTree left() const { DCHECK(node_); return {node_->left}; }
    RedBlackTree right() const { DCHECK(node_); return {node_->right}; }
    Color color() const { DCHECK(node_); return node_->color; }
    // clang-format on

    // Mutators.
    RedBlackTree insert(const NodeData& x, size_t at) const;
    RedBlackTree remove(size_t at) const;

    // Helpers.
    bool operator==(const RedBlackTree&) const = default;

private:
    NodePtr node_;
};

}  // namespace editor
