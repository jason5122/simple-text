#pragma once

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
};

enum class Color { Red, Black, DoubleBlack };

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
    constexpr const Node* root_ptr() const { return root_node_.get(); }
    constexpr bool empty() const { return !root_node_; }
    const NodeData& data() const;
    RedBlackTree left() const;
    RedBlackTree right() const;
    Color root_color() const;
    size_t length() const;
    size_t lf_count() const;

    // Helpers.
    bool operator==(const RedBlackTree&) const = default;

    // Mutators.
    RedBlackTree insert(const NodeData& x, size_t at) const;
    RedBlackTree remove(size_t at) const;

    // TODO: Consider making these private.
    RedBlackTree paint(Color c) const;

private:
    RedBlackTree internal_insert(const NodeData& x, size_t at, size_t total_offset) const;

    NodePtr root_node_;
};

}  // namespace editor
