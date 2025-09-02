#include "base/buffer/piece_tree_rbtree.h"
#include "base/check.h"
#include <cassert>

namespace base {

RedBlackTree::Node::Node(Color c, const NodePtr& lft, const NodeData& data, const NodePtr& rgt)
    : color(c), left(lft), data(data), right(rgt) {}

const RedBlackTree::Node* RedBlackTree::root_ptr() const { return root_node.get(); }

bool RedBlackTree::empty() const { return !root_node; }

const NodeData& RedBlackTree::data() const {
    assert(!empty());
    return root_node->data;
}

RedBlackTree RedBlackTree::left() const {
    assert(!empty());
    return RedBlackTree(root_node->left);
}

RedBlackTree RedBlackTree::right() const {
    assert(!empty());
    return RedBlackTree(root_node->right);
}

Color RedBlackTree::root_color() const {
    assert(!empty());
    return root_node->color;
}

RedBlackTree RedBlackTree::insert(const NodeData& x, size_t at) const {
    RedBlackTree t = internal_insert(x, at, 0);
    return RedBlackTree(Color::Black, t.left(), t.data(), t.right());
}

NodeData attribute(const NodeData& data, const RedBlackTree& left) {
    NodeData new_data = data;
    new_data.left_subtree_length = left.length();
    new_data.left_subtree_lf_count = left.lf_count();
    return new_data;
}

RedBlackTree::RedBlackTree(Color c,
                           const RedBlackTree& lft,
                           const NodeData& val,
                           const RedBlackTree& rgt)
    : root_node(std::make_shared<Node>(c, lft.root_node, attribute(val, lft), rgt.root_node)) {}

RedBlackTree::RedBlackTree(const NodePtr& node) : root_node(node) {}

RedBlackTree RedBlackTree::internal_insert(const NodeData& x,
                                           size_t at,
                                           size_t total_offset) const {
    if (empty()) {
        return RedBlackTree(Color::Red, RedBlackTree(), x, RedBlackTree());
    }

    const NodeData& y = data();
    if (at < total_offset + y.left_subtree_length + y.piece.length) {
        return balance(root_color(), left().internal_insert(x, at, total_offset), y, right());
    }
    auto rgt =
        right().internal_insert(x, at, total_offset + y.left_subtree_length + y.piece.length);
    return balance(root_color(), left(), y, rgt);
}

RedBlackTree RedBlackTree::balance(Color c,
                                   const RedBlackTree& lft,
                                   const NodeData& x,
                                   const RedBlackTree& rgt) {
    if (c == Color::Black && lft.doubled_left())
        return RedBlackTree(Color::Red, lft.left().paint(Color::Black), lft.data(),
                            RedBlackTree(Color::Black, lft.right(), x, rgt));
    else if (c == Color::Black && lft.doubled_right())
        return RedBlackTree(
            Color::Red, RedBlackTree(Color::Black, lft.left(), lft.data(), lft.right().left()),
            lft.right().data(), RedBlackTree(Color::Black, lft.right().right(), x, rgt));
    else if (c == Color::Black && rgt.doubled_left())
        return RedBlackTree(
            Color::Red, RedBlackTree(Color::Black, lft, x, rgt.left().left()), rgt.left().data(),
            RedBlackTree(Color::Black, rgt.left().right(), rgt.data(), rgt.right()));
    else if (c == Color::Black && rgt.doubled_right())
        return RedBlackTree(Color::Red, RedBlackTree(Color::Black, lft, x, rgt.left()), rgt.data(),
                            rgt.right().paint(Color::Black));
    return RedBlackTree(c, lft, x, rgt);
}

bool RedBlackTree::doubled_left() const {
    return !empty() && root_color() == Color::Red && !left().empty() &&
           left().root_color() == Color::Red;
}

bool RedBlackTree::doubled_right() const {
    return !empty() && root_color() == Color::Red && !right().empty() &&
           right().root_color() == Color::Red;
}

RedBlackTree RedBlackTree::paint(Color c) const {
    assert(!empty());
    return RedBlackTree(c, left(), data(), right());
}

size_t RedBlackTree::length() const {
    if (empty()) return {};
    return data().left_subtree_length + data().piece.length + right().length();
}

size_t RedBlackTree::lf_count() const {
    if (empty()) return {};
    return data().left_subtree_lf_count + data().piece.newline_count + right().lf_count();
}

struct WalkResult {
    RedBlackTree tree;
    size_t accumulated_offset;
};

WalkResult pred(const RedBlackTree& root, size_t start_offset) {
    RedBlackTree t = root.left();
    while (!t.right().empty()) {
        start_offset = start_offset + t.data().left_subtree_length + t.data().piece.length;
        t = t.right();
    }
    // Add the final offset from the last right node.
    start_offset = start_offset + t.data().left_subtree_length;
    return {.tree = t, .accumulated_offset = start_offset};
}

RedBlackTree RedBlackTree::remove(size_t at) const {
    auto t = rem(*this, at, 0);
    if (t.empty()) return RedBlackTree();
    return RedBlackTree(Color::Black, t.left(), t.data(), t.right());
}

RedBlackTree RedBlackTree::fuse(const RedBlackTree& left, const RedBlackTree& right) {
    // match: (left, right)
    // case: (None, r)
    if (left.empty()) return right;
    if (right.empty()) return left;
    // match: (left.color, right.color)
    // case: (B, R)
    if (left.root_color() == Color::Black && right.root_color() == Color::Red) {
        return RedBlackTree(Color::Red, fuse(left, right.left()), right.data(), right.right());
    }
    // case: (R, B)
    if (left.root_color() == Color::Red && right.root_color() == Color::Black) {
        return RedBlackTree(Color::Red, left.left(), left.data(), fuse(left.right(), right));
    }
    // case: (R, R)
    if (left.root_color() == Color::Red && right.root_color() == Color::Red) {
        auto fused = fuse(left.right(), right.left());
        if (!fused.empty() && fused.root_color() == Color::Red) {
            auto new_left = RedBlackTree(Color::Red, left.left(), left.data(), fused.left());
            auto new_right = RedBlackTree(Color::Red, fused.right(), right.data(), right.right());
            return RedBlackTree(Color::Red, new_left, fused.data(), new_right);
        }
        auto new_right = RedBlackTree(Color::Red, fused, right.data(), right.right());
        return RedBlackTree(Color::Red, left.left(), left.data(), new_right);
    }
    // case: (B, B)
    assert(left.root_color() == right.root_color() && left.root_color() == Color::Black);
    auto fused = fuse(left.right(), right.left());
    if (!fused.empty() && fused.root_color() == Color::Red) {
        auto new_left = RedBlackTree(Color::Black, left.left(), left.data(), fused.left());
        auto new_right = RedBlackTree(Color::Black, fused.right(), right.data(), right.right());
        return RedBlackTree(Color::Red, new_left, fused.data(), new_right);
    }
    auto new_right = RedBlackTree(Color::Black, fused, right.data(), right.right());
    auto new_node = RedBlackTree(Color::Red, left.left(), left.data(), new_right);
    return balance_left(new_node);
}

RedBlackTree RedBlackTree::balance(const RedBlackTree& node) {
    // Two red children.
    if (!node.left().empty() && node.left().root_color() == Color::Red && !node.right().empty() &&
        node.right().root_color() == Color::Red) {
        auto l = node.left().paint(Color::Black);
        auto r = node.right().paint(Color::Black);
        return RedBlackTree(Color::Red, l, node.data(), r);
    }

    assert(node.root_color() == Color::Black);
    return balance(node.root_color(), node.left(), node.data(), node.right());
}

RedBlackTree RedBlackTree::balance_left(const RedBlackTree& left) {
    // match: (color_l, color_r, color_r_l)
    // case: (Some(R), ..)
    if (!left.left().empty() && left.left().root_color() == Color::Red) {
        return RedBlackTree(Color::Red, left.left().paint(Color::Black), left.data(),
                            left.right());
    }
    // case: (_, Some(B), _)
    if (!left.right().empty() && left.right().root_color() == Color::Black) {
        auto new_left =
            RedBlackTree(Color::Black, left.left(), left.data(), left.right().paint(Color::Red));
        return balance(new_left);
    }
    // case: (_, Some(R), Some(B))
    if (!left.right().empty() && left.right().root_color() == Color::Red &&
        !left.right().left().empty() && left.right().left().root_color() == Color::Black) {
        auto unbalanced_new_right =
            RedBlackTree(Color::Black, left.right().left().right(), left.right().data(),
                         left.right().right().paint(Color::Red));
        auto new_right = balance(unbalanced_new_right);
        auto new_left =
            RedBlackTree(Color::Black, left.left(), left.data(), left.right().left().left());
        return RedBlackTree(Color::Red, new_left, left.right().left().data(), new_right);
    }
    NOTREACHED();
}

RedBlackTree RedBlackTree::balance_right(const RedBlackTree& right) {
    // match: (color_l, color_l_r, color_r)
    // case: (.., Some(R))
    if (!right.right().empty() && right.right().root_color() == Color::Red) {
        return RedBlackTree(Color::Red, right.left(), right.data(),
                            right.right().paint(Color::Black));
    }
    // case: (Some(B), ..)
    if (!right.left().empty() && right.left().root_color() == Color::Black) {
        auto new_right = RedBlackTree(Color::Black, right.left().paint(Color::Red), right.data(),
                                      right.right());
        return balance(new_right);
    }
    // case: (Some(R), Some(B), _)
    if (!right.left().empty() && right.left().root_color() == Color::Red &&
        !right.left().right().empty() && right.left().right().root_color() == Color::Black) {
        auto unbalanced_new_left =
            RedBlackTree(Color::Black,
                         // Note: Because 'left' is red, it must have a left child.
                         right.left().left().paint(Color::Red), right.left().data(),
                         right.left().right().left());
        auto new_left = balance(unbalanced_new_left);
        auto new_right =
            RedBlackTree(Color::Black, right.left().right().right(), right.data(), right.right());
        return RedBlackTree(Color::Red, new_left, right.left().right().data(), new_right);
    }
    NOTREACHED();
}

RedBlackTree RedBlackTree::remove_left(const RedBlackTree& root, size_t at, size_t total) {
    auto new_left = rem(root.left(), at, total);
    auto new_node = RedBlackTree(Color::Red, new_left, root.data(), root.right());
    // In this case, the root was a red node and must've had at least two children.
    if (!root.left().empty() && root.left().root_color() == Color::Black)
        return balance_left(new_node);
    return new_node;
}

RedBlackTree RedBlackTree::remove_right(const RedBlackTree& root, size_t at, size_t total) {
    const NodeData& y = root.data();
    auto new_right = rem(root.right(), at, total + y.left_subtree_length + y.piece.length);
    auto new_node = RedBlackTree(Color::Red, root.left(), root.data(), new_right);
    // In this case, the root was a red node and must've had at least two children.
    if (!root.right().empty() && root.right().root_color() == Color::Black)
        return balance_right(new_node);
    return new_node;
}

RedBlackTree RedBlackTree::rem(const RedBlackTree& root, size_t at, size_t total) {
    if (root.empty()) return RedBlackTree();
    const NodeData& y = root.data();
    if (at < total + y.left_subtree_length) return remove_left(root, at, total);
    if (at == total + y.left_subtree_length) return fuse(root.left(), root.right());
    return remove_right(root, at, total);
}

}  // namespace base
