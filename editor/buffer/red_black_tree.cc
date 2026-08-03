#include "base/check.h"
#include "editor/buffer/red_black_tree.h"

namespace editor {

using Color = RedBlackTree::Color;

RedBlackTree::RedBlackTree(Color c,
                           const RedBlackTree& left,
                           const Piece& p,
                           const RedBlackTree& right) {
    NodeData d = {
        .piece = p,
        .left_length = left.length(),
        .left_lf_count = left.line_feed_count(),
        .subtree_length = left.length() + p.length + right.length(),
        .subtree_lf_count = left.line_feed_count() + p.lf_count + right.line_feed_count(),
    };
    node_ = std::make_shared<Node>(c, left.node_, d, right.node_);
}

// =================================================================================================
// Insertion
// =================================================================================================

namespace {
RedBlackTree ins(const RedBlackTree& node, const Piece& p, size_t at, size_t total_offset) {
    if (!node) return {Color::Red, {}, p, {}};

    size_t split = total_offset + node.left_length() + node.piece().length;
    if (at < split) {
        auto new_left = ins(node.left(), p, at, total_offset);
        RedBlackTree rebuilt = {node.color(), new_left, node.piece(), node.right()};
        return rebuilt.balance();
    } else {
        auto new_right = ins(node.right(), p, at, split);
        RedBlackTree rebuilt = {node.color(), node.left(), node.piece(), new_right};
        return rebuilt.balance();
    }
}
}  // namespace

RedBlackTree RedBlackTree::insert(size_t at, const Piece& p) const {
    return ins(*this, p, at, 0).blacken();
}

// See okasaki_balance.png (Okasaki, 1999, Fig. 1) for the balance cases.
RedBlackTree RedBlackTree::balance() const {
    if (empty() || is_red()) return *this;

    auto l = left();
    auto r = right();
    const auto& p = piece();

    if (l.double_red_left()) {
        return {Color::Red, l.left().blacken(), l.piece(), {Color::Black, l.right(), p, r}};
    }
    if (l.double_red_right()) {
        return {Color::Red,
                {Color::Black, l.left(), l.piece(), l.right().left()},
                l.right().piece(),
                {Color::Black, l.right().right(), p, r}};
    }
    if (r.double_red_left()) {
        return {Color::Red,
                {Color::Black, l, p, r.left().left()},
                r.left().piece(),
                {Color::Black, r.left().right(), r.piece(), r.right()}};
    }
    if (r.double_red_right()) {
        return {Color::Red, {Color::Black, l, p, r.left()}, r.piece(), r.right().blacken()};
    }
    return *this;
}

// =================================================================================================
// Deletion
// =================================================================================================
namespace {

RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right) {
    // match: (left, right)
    // case: (None, r)
    if (!left) return right;
    if (!right) return left;
    // match: (left.color, right.color)
    // case: (B, R)
    if (left.is_black() && right.is_red()) {
        return {Color::Red, fuse(left, right.left()), right.piece(), right.right()};
    }
    // case: (R, B)
    if (left.is_red() && right.is_black()) {
        return {Color::Red, left.left(), left.piece(), fuse(left.right(), right)};
    }
    // case: (R, R)
    if (left.is_red() && right.is_red()) {
        auto fused = fuse(left.right(), right.left());
        if (fused.is_red()) {
            RedBlackTree new_left = {Color::Red, left.left(), left.piece(), fused.left()};
            RedBlackTree new_right = {Color::Red, fused.right(), right.piece(), right.right()};
            return {Color::Red, new_left, fused.piece(), new_right};
        }
        RedBlackTree new_right = {Color::Red, fused, right.piece(), right.right()};
        return {Color::Red, left.left(), left.piece(), new_right};
    }
    // case: (B, B)
    DCHECK(left.is_black() && right.is_black());
    auto fused = fuse(left.right(), right.left());
    if (fused.is_red()) {
        RedBlackTree new_left = {Color::Black, left.left(), left.piece(), fused.left()};
        RedBlackTree new_right = {Color::Black, fused.right(), right.piece(), right.right()};
        return {Color::Red, new_left, fused.piece(), new_right};
    }
    RedBlackTree new_right = {Color::Black, fused, right.piece(), right.right()};
    RedBlackTree new_node = {Color::Red, left.left(), left.piece(), new_right};
    return new_node.balance_left();
}

RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total) {
    if (!root) return {};
    if (at < total + root.left_length()) return root.remove_left(at, total);
    if (at == total + root.left_length()) return fuse(root.left(), root.right());
    return root.remove_right(at, total);
}

}  // namespace

RedBlackTree RedBlackTree::remove(size_t at) const {
    auto t = rem(*this, at, 0);
    if (!t) return {};
    return t.blacken();
}

RedBlackTree RedBlackTree::flip_children_if_both_red() const {
    DCHECK(is_black());
    // Both children are red.
    if (left().is_red() && right().is_red()) {
        return {Color::Red, left().blacken(), piece(), right().blacken()};
    }
    return balance();
}

RedBlackTree RedBlackTree::balance_left() const {
    // match: (color_l, color_r, color_r_l)
    // case: (Some(R), ..)
    if (left() && left().is_red()) {
        return {Color::Red, left().blacken(), piece(), right()};
    }
    // case: (_, Some(B), _)
    if (right() && right().is_black()) {
        RedBlackTree new_left = {Color::Black, left(), piece(), right().redden()};
        return new_left.flip_children_if_both_red();
    }
    // case: (_, Some(R), Some(B))
    if (right() && right().is_red() && right().left() && right().left().is_black()) {
        RedBlackTree unbalanced_new_right = {
            Color::Black,
            right().left().right(),
            right().piece(),
            right().right().redden(),
        };
        auto new_right = unbalanced_new_right.flip_children_if_both_red();
        RedBlackTree new_left = {Color::Black, left(), piece(), right().left().left()};
        return {Color::Red, new_left, right().left().piece(), new_right};
    }
    NOTREACHED();
}

RedBlackTree RedBlackTree::balance_right() const {
    // match: (color_l, color_l_r, color_r)
    // case: (.., Some(R))
    if (right() && right().is_red()) {
        return {Color::Red, left(), piece(), right().blacken()};
    }
    // case: (Some(B), ..)
    if (left() && left().is_black()) {
        RedBlackTree new_right = {Color::Black, left().redden(), piece(), right()};
        return new_right.flip_children_if_both_red();
    }
    // case: (Some(R), Some(B), _)
    if (left() && left().is_red() && left().right() && left().right().is_black()) {
        RedBlackTree unbalanced_new_left = {
            Color::Black,
            // Note: Because 'left' is red, it must have a left child.
            left().left().redden(),
            left().piece(),
            left().right().left(),
        };
        auto new_left = unbalanced_new_left.flip_children_if_both_red();
        RedBlackTree new_right = {Color::Black, left().right().right(), piece(), right()};
        return {Color::Red, new_left, left().right().piece(), new_right};
    }
    NOTREACHED();
}

RedBlackTree RedBlackTree::remove_left(size_t at, size_t total) const {
    auto new_left = rem(left(), at, total);
    RedBlackTree new_node = {Color::Red, new_left, piece(), right()};
    // In this case, the root was a red node and must've had at least two children.
    if (left() && left().is_black()) {
        return new_node.balance_left();
    }
    return new_node;
}

RedBlackTree RedBlackTree::remove_right(size_t at, size_t total) const {
    auto new_right = rem(right(), at, total + left_length() + piece().length);
    RedBlackTree new_node = {Color::Red, left(), piece(), new_right};
    // In this case, the root was a red node and must've had at least two children.
    if (right() && right().is_black()) {
        return new_node.balance_right();
    }
    return new_node;
}

// =================================================================================================
// Debugging
// =================================================================================================
namespace {
// Borrowed from https://github.com/dotnwat/persistent-rbtree/blob/master/tree.h:checkConsistency.
int black_height_or_zero(const RedBlackTree& node) {
    if (!node) return 1;
    if (node.double_red_left() || node.double_red_right()) return 0;

    auto l = black_height_or_zero(node.left());
    auto r = black_height_or_zero(node.right());
    if (!l || !r || l != r) return 0;
    return node.is_red() ? l : l + 1;
}
}  // namespace

bool RedBlackTree::satisfies_red_black_invariants() const {
    // 1. Each node is red or black.
    // 2. Red nodes have black children.
    // 3. Every path from a node to an empty leaf has the same black height.
    // 4. Leaves are implicitly black.
    return black_height_or_zero(*this) != 0;
}

}  // namespace editor
