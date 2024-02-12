#pragma once

#include <memory>

#include "types.h"

namespace PieceTree {
enum class BufferIndex : size_t { ModBuf = sentinel_for<BufferIndex> };

enum class Line : size_t { IndexBeginning, Beginning };

using Editor::CharOffset;
using Editor::Column;
using Editor::Length;

enum class LFCount : size_t {};

struct BufferCursor {
    // Relative line in the current buffer.
    Line line = {};
    // Column into the current line.
    Column column = {};

    bool operator==(const BufferCursor&) const = default;
};

struct Piece {
    BufferIndex index = {};  // Index into a buffer in PieceTree.  This could be an immutable
                             // buffer or the mutable buffer.
    BufferCursor first = {};
    BufferCursor last = {};
    Length length = {};
    LFCount newline_count = {};
};

using Offset = PieceTree::CharOffset;

struct NodeData {
    PieceTree::Piece piece;

    PieceTree::Length left_subtree_length = {};
    PieceTree::LFCount left_subtree_lf_count = {};
};

class RedBlackTree;

NodeData attribute(const NodeData& data, const RedBlackTree& left);

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
    struct ColorTree;

    explicit RedBlackTree() = default;

    // Queries.
    const Node* root_ptr() const;
    bool is_empty() const;
    const NodeData& root() const;
    RedBlackTree left() const;
    RedBlackTree right() const;
    Color root_color() const;

    // Helpers.
    bool operator==(const RedBlackTree&) const = default;

    // Mutators.
    RedBlackTree insert(const NodeData& x, Offset at) const;
    RedBlackTree remove(Offset at) const;

private:
    RedBlackTree(Color c, const RedBlackTree& lft, const NodeData& val, const RedBlackTree& rgt);

    RedBlackTree(const NodePtr& node);

    // Removal.
    static RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right);
    static RedBlackTree balance(const RedBlackTree& node);
    static RedBlackTree balance_left(const RedBlackTree& left);
    static RedBlackTree balance_right(const RedBlackTree& right);
    static RedBlackTree remove_left(const RedBlackTree& root, Offset at, Offset total);
    static RedBlackTree remove_right(const RedBlackTree& root, Offset at, Offset total);
    static RedBlackTree rem(const RedBlackTree& root, Offset at, Offset total);

    // Insertion.
    RedBlackTree ins(const NodeData& x, Offset at, Offset total_offset) const;
    static RedBlackTree balance(Color c, const RedBlackTree& lft, const NodeData& x,
                                const RedBlackTree& rgt);
    bool doubled_left() const;
    bool doubled_right() const;

    // General.
    RedBlackTree paint(Color c) const;

    NodePtr root_node;
};

// Global queries.
PieceTree::Length tree_length(const RedBlackTree& root);
PieceTree::LFCount tree_lf_count(const RedBlackTree& root);
}  // namespace PieceTree
