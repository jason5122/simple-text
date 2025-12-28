#include "base/check.h"
#include "editor/buffer/red_black_tree.h"

namespace editor {

namespace {

RedBlackTree internal_insert(const RedBlackTree& node,
                             const Piece& x,
                             size_t at,
                             size_t total_offset);
RedBlackTree internal_balance(Color c,
                              const RedBlackTree& left,
                              const Piece& p,
                              const RedBlackTree& right);
RedBlackTree fuse(const RedBlackTree& left, const RedBlackTree& right);
RedBlackTree rem(const RedBlackTree& root, size_t at, size_t total);

}  // namespace

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

RedBlackTree RedBlackTree::insert(size_t at, const Piece& p) const {
    return internal_insert(*this, p, at, 0).blacken();
}

RedBlackTree RedBlackTree::remove(size_t at) const {
    auto t = rem(*this, at, 0);
    if (!t) return {};
    return t.blacken();
}

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
    // 1. Every node is either red or black.
    // 2. The root is black.
    // 3. Every leaf (NIL) is black.
    // 4. If a node is red, then both its children are black.
    // 5. For each node, all simple paths from the node to descendant leaves contain the same
    // number of black nodes.

    // The internal nodes in this RB tree can be totally black so we will not count them directly,
    // we'll just track odd nodes as either red or black. Measure the number of black nodes we need
    // to validate.
    const auto& root = *this;
    if (!root.is_black()) return false;
    return black_height_or_zero(root) != 0;
}

RedBlackTree RedBlackTree::balance() const {
    DCHECK(is_black());
    // Both children are red.
    if (left().is_red() && right().is_red()) {
        return {Color::Red, left().blacken(), piece(), right().blacken()};
    }
    return internal_balance(color(), left(), piece(), right());
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
        return new_left.balance();
    }
    // case: (_, Some(R), Some(B))
    if (right() && right().is_red() && right().left() && right().left().is_black()) {
        RedBlackTree unbalanced_new_right = {
            Color::Black,
            right().left().right(),
            right().piece(),
            right().right().redden(),
        };
        auto new_right = unbalanced_new_right.balance();
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
        return new_right.balance();
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
        auto new_left = unbalanced_new_left.balance();
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

namespace {

RedBlackTree internal_insert(const RedBlackTree& node,
                             const Piece& p,
                             size_t at,
                             size_t total_offset) {
    if (!node) return {Color::Red, {}, p, {}};

    // TODO: Maybe dedup `total_offset + node.left_length() + node.piece().length`?
    // TODO: Could this function be written more cleanly, or is this crystal clear? Could we leave
    // some comments to explain?
    if (at < total_offset + node.left_length() + node.piece().length) {
        return internal_balance(node.color(), internal_insert(node.left(), p, at, total_offset),
                                node.piece(), node.right());
    }
    auto right = internal_insert(node.right(), p, at,
                                 total_offset + node.left_length() + node.piece().length);
    return internal_balance(node.color(), node.left(), node.piece(), right);
}

// TODO: Make the ASCII diagrams cleaner. They're not really symmetric lol
RedBlackTree internal_balance(Color c,
                              const RedBlackTree& left,
                              const Piece& p,
                              const RedBlackTree& right) {
    /*
     * Balanced (goal):
     *
     *     R(y)
     *    /    \
     *   B(x)  B(z)
     */

    /*
     * Case LL:
     *
     *       B(z)
     *      /
     *     R(y)
     *    /
     *   R(x)
     */
    if (c == Color::Black && left.double_red_left()) {
        return {Color::Red,
                left.left().blacken(),
                left.piece(),
                {Color::Black, left.right(), p, right}};
    }
    /*
     * Case LR:
     *
     *     B(z)
     *    /
     *   R(x)
     *    \
     *     R(y)
     */
    if (c == Color::Black && left.double_red_right()) {
        return {Color::Red,
                {Color::Black, left.left(), left.piece(), left.right().left()},
                left.right().piece(),
                {Color::Black, left.right().right(), p, right}};
    }
    /*
     * Case RL:
     *
     *     B(x)
     *         \
     *        R(z)
     *         /
     *     R(y)
     */
    if (c == Color::Black && right.double_red_left()) {
        return {Color::Red,
                {Color::Black, left, p, right.left().left()},
                right.left().piece(),
                {Color::Black, right.left().right(), right.piece(), right.right()}};
    }
    /*
     * Case RR:
     *
     *     B(x)
     *         \
     *        R(y)
     *           \
     *           R(y)
     */
    if (c == Color::Black && right.double_red_right()) {
        return {Color::Red,
                {Color::Black, left, p, right.left()},
                right.piece(),
                right.right().blacken()};
    }
    return {c, left, p, right};
}

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

}  // namespace editor
