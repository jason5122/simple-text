#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace btree {

// A monoid: S{} is the identity and + is associative, neither of which the compiler can check.
// == lets check_invariants compare a stored summary with a recomputed one.
template <typename S>
concept Summary = std::default_initializable<S> && std::equality_comparable<S> &&
                  requires(const S& a, const S& b) {
                      { a + b } -> std::same_as<S>;
                  };

// An item that measures itself: summary() returns its Summary by value.
template <typename Item>
concept Measured = requires(const Item& item) {
    { item.summary() } -> Summary;
};

// A B+-tree over a sequence, augmented as CLRS §14.2 describes: every entry carries a summary of
// what lies beneath it, and the tree is navigated by those summaries rather than by keys. Items
// measure themselves and the tree sums the measures. With a summary of {count} this is the
// order-statistic tree of CLRS §14.1.
//
// t is the minimum degree, as in the B-tree and the B+-tree: every node but the root holds
// between t and 2t - 1 entries. The default keeps trees small enough to trace by hand; a tree
// built for speed should pick t from a measurement.
template <Measured Item, int t = 2>
class SumTree {
public:
    using S = std::remove_cvref_t<decltype(std::declval<const Item&>().summary())>;

    // Leaves hold items and nonleaf nodes hold children; either way a node holds n entries with
    // a summary each, the item's own or that of the whole subtree under the child. One slot more
    // than a node may hold, so an insert can overflow in place and then split.
    struct Node {
        bool leaf = true;
        int n = 0;
        std::array<S, 2 * t> summary;
        std::array<Item, 2 * t> item;                // Leaf only.
        std::array<std::unique_ptr<Node>, 2 * t> c;  // Nonleaf only.
    };

    // A position in the sequence: the path from the root down to an item, and the summary of
    // everything before that item. One past the last item is end(). Items are read-only through
    // it, since the tree caches their summaries; replace() is the way to change one.
    class iterator {
    public:
        using value_type = Item;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;

        iterator() = default;

        const S& start() const { return start_; }
        const Item& operator*() const { return leaf().node->item[leaf().index]; }  // Not end().
        const Item* operator->() const { return &**this; }

        // Steps past the current item. Not end().
        iterator& operator++() {
            Step& step = leaf();
            start_ = start_ + step.node->summary[step.index];
            step.index++;
            if (step.index < step.node->n) return *this;
            // The leaf is used up: climb to the nearest ancestor with a further child and go
            // down that child's leftmost branch. With no such ancestor this is the end.
            int d = depth_ - 1;
            while (d > 0 && path_[d - 1].index + 1 == path_[d - 1].node->n) d--;
            if (d == 0) return *this;
            depth_ = d;
            Step& up = path_[d - 1];
            up.index++;
            descend_leftmost(up.node->c[up.index].get());
            return *this;
        }
        iterator operator++(int) {
            iterator old = *this;
            ++*this;
            return old;
        }

        // Positions in one tree are equal when they name the same slot of the same leaf. In an
        // empty tree every position is the end.
        bool operator==(const iterator& other) const {
            if (depth_ == 0 || other.depth_ == 0) return depth_ == other.depth_;
            return leaf().node == other.leaf().node && leaf().index == other.leaf().index;
        }

    private:
        friend class SumTree;

        struct Step {
            Node* node;
            int index;  // The entry the path takes at this node; in the leaf, the item itself.
        };

        Step& leaf() { return path_[depth_ - 1]; }
        const Step& leaf() const { return path_[depth_ - 1]; }
        void push(Node* node, int index) { path_[depth_++] = {node, index}; }
        // Extends the path from x down first children to a leaf's first item.
        void descend_leftmost(Node* x) {
            while (!x->leaf) {
                push(x, 0);
                x = x->c[0].get();
            }
            push(x, 0);
        }

        S start_{};
        // Every node but the root holds at least two entries, so a tree of 2^31 items is at
        // most 31 levels deep.
        std::array<Step, 32> path_;
        int depth_ = 0;
    };
    using const_iterator = iterator;

    SumTree() = default;
    // Adopts a hand-built tree, so tests can construct exact shapes node by node.
    explicit SumTree(std::unique_ptr<Node> root) : root_{std::move(root)} {}

    S summary() const { return root_ == nullptr ? S{} : total(*root_); }

    // Finds the first item whose end, the summary of the sequence through that item, satisfies
    // pred. pred must be monotone along the sequence: false up to some item and true from there
    // on. With no such item the result is end(). With S = {count}, count >= k finds the k-th
    // item, CLRS's OS-SELECT, and the iterator's start() is its rank less one, OS-RANK.
    template <std::predicate<const S&> Pred>
    iterator seek(Pred pred) const {
        iterator it;
        if (root_ == nullptr) return it;
        Node* x = root_.get();
        while (true) {
            // Pass over the entries whose end pred rejects, accumulating them into start.
            int i = 0;
            for (; i < x->n; i++) {
                S entry_end = it.start_ + x->summary[i];
                if (pred(entry_end)) break;
                it.start_ = entry_end;
            }
            // No entry reaches the target, which a monotone pred allows only at the root: the
            // target lies beyond the sequence.
            if (i == x->n) return end();
            it.push(x, i);
            if (x->leaf) return it;
            x = x->c[i].get();
        }
    }

    iterator begin() const {
        iterator it;
        if (root_ != nullptr) it.descend_leftmost(root_.get());
        return it;
    }

    iterator end() const {
        iterator it;
        if (root_ == nullptr) return it;
        it.start_ = total(*root_);
        Node* x = root_.get();
        while (!x->leaf) {
            it.push(x, x->n - 1);
            x = x->c[x->n - 1].get();
        }
        it.push(x, x->n);
        return it;
    }

    // Inserts the item before the iterator's position, so at the end for end(). Edits
    // invalidate iterators, except that replace leaves them valid.
    void insert(const iterator& before, Item item) {
        iterator at = before;
        if (root_ == nullptr) {
            root_ = std::make_unique<Node>();
            at.push(root_.get(), 0);
        }
        Step& step = at.leaf();
        Node& L = *step.node;
        open_slot(L, step.index);
        L.summary[step.index] = item.summary();
        L.item[step.index] = std::move(item);
        if (L.n == 2 * t) {
            split(at, at.depth_ - 1);
        } else {
            refresh(at, at.depth_ - 1);
        }
    }

    void replace(const iterator& at, Item item) {  // Not end().
        const Step& step = at.leaf();
        step.node->summary[step.index] = item.summary();
        step.node->item[step.index] = std::move(item);
        refresh(at, at.depth_ - 1);
    }

    void erase(const iterator& at) {  // Not end().
        iterator path = at;
        delete_entry(path, path.depth_ - 1);
    }

    std::vector<Item> items() const {
        std::vector<Item> out;
        if (root_ != nullptr) collect(*root_, out);
        return out;
    }

    bool check_invariants() const { return root_ == nullptr || check_subtree(*root_, true) >= 0; }

    const Node* root() const { return root_.get(); }

private:
    using Step = typename iterator::Step;

    static S total(const Node& x) {
        S sum;
        for (int i = 0; i < x.n; i++) sum = sum + x.summary[i];
        return sum;
    }

    // Moves entry i of `from` into slot j of `to`, nodes of the same kind or the same node.
    static void move_entry(Node& from, int i, Node& to, int j) {
        to.summary[j] = from.summary[i];
        if (from.leaf) {
            to.item[j] = std::move(from.item[i]);
        } else {
            to.c[j] = std::move(from.c[i]);
        }
    }

    // Opens a slot at i for the caller to fill, shifting the entries from i on to the right.
    static void open_slot(Node& x, int i) {
        for (int j = x.n; j > i; j--) move_entry(x, j - 1, x, j);
        x.n++;
    }

    // Drops entry i, shifting the entries after it to the left.
    static void close_slot(Node& x, int i) {
        for (int j = i; j + 1 < x.n; j++) move_entry(x, j + 1, x, j);
        x.n--;
        if (!x.leaf) x.c[x.n] = nullptr;  // Null already if moved from, else the dropped child.
    }

    // Recomputes, at every ancestor above `level`, the summary of the child the path goes
    // through. A node's summary depends only on its own entries (CLRS Theorem 14.1), so an edit
    // is repaired by re-summing the path.
    static void refresh(const iterator& path, int level) {
        for (int d = level; d > 0; d--) {
            const Step& up = path.path_[d - 1];
            up.node->summary[up.index] = total(*up.node->c[up.index]);
        }
    }

    // The node x at `level` of the path holds 2t entries, one more than it may: moves its upper
    // half into a new node x′ and puts x′ after x in the parent, splitting the parent in turn if
    // it overflows. The B+-tree's insert_in_parent (Figure 14.17) without the separator keys.
    void split(const iterator& path, int level) {
        Node& x = *path.path_[level].node;
        auto x_prime = std::make_unique<Node>();
        x_prime->leaf = x.leaf;
        for (int i = t; i < 2 * t; i++) move_entry(x, i, *x_prime, i - t);
        x.n = t;
        x_prime->n = t;
        if (level == 0) {
            // x was the root: a new root holds x and x′.
            auto R = std::make_unique<Node>();
            R->leaf = false;
            R->n = 2;
            R->summary[0] = total(x);
            R->summary[1] = total(*x_prime);
            R->c[0] = std::move(root_);
            R->c[1] = std::move(x_prime);
            root_ = std::move(R);
            return;
        }
        const Step& up = path.path_[level - 1];
        Node& P = *up.node;
        int j = up.index;  // x is P.c[j].
        P.summary[j] = total(x);
        open_slot(P, j + 1);
        P.summary[j + 1] = total(*x_prime);
        P.c[j + 1] = std::move(x_prime);
        if (P.n == 2 * t) {
            split(path, level - 1);
        } else {
            refresh(path, level - 1);
        }
    }

    // Removes the entry the path takes at `level` from its node N, then restores N's minimum by
    // borrowing from a sibling or merging with one, which removes an entry from the parent in
    // turn. The B+-tree's delete_entry (Figure 14.21) without the separator keys: with uniform
    // entries a borrow moves one entry across, and a merge appends the right node to the left.
    void delete_entry(iterator& path, int level) {
        Node& N = *path.path_[level].node;
        close_slot(N, path.path_[level].index);
        if (level == 0) {
            if (!N.leaf && N.n == 1) {
                // The root has one child left, which becomes the root.
                std::unique_ptr<Node> child = std::move(N.c[0]);
                root_ = std::move(child);
            } else if (N.n == 0) {
                root_ = nullptr;  // The last item is gone.
            }
            return;
        }
        if (N.n >= t) {
            refresh(path, level);
            return;
        }
        // N has too few entries. Pair it with a sibling, preferring the one on its left: L and R
        // are the pair in order.
        Step& up = path.path_[level - 1];
        Node& P = *up.node;
        int left = up.index > 0 ? up.index - 1 : up.index;
        Node& L = *P.c[left];
        Node& R = *P.c[left + 1];
        Node& sibling = &L == &N ? R : L;
        if (sibling.n > t) {
            // Borrow the sibling's entry nearest to N, into N's near end.
            if (&sibling == &L) {
                open_slot(N, 0);
                move_entry(L, L.n - 1, N, 0);
                close_slot(L, L.n - 1);
            } else {
                open_slot(N, N.n);
                move_entry(R, 0, N, N.n - 1);
                close_slot(R, 0);
            }
            P.summary[left] = total(L);
            P.summary[left + 1] = total(R);
            refresh(path, level - 1);
        } else {
            // Merge: append R to L, then remove R's entry from P, which destroys R.
            for (int i = 0; i < R.n; i++) move_entry(R, i, L, L.n + i);
            L.n += R.n;
            P.summary[left] = total(L);
            up.index = left + 1;
            delete_entry(path, level - 1);
        }
    }

    static void collect(const Node& x, std::vector<Item>& out) {
        for (int i = 0; i < x.n; i++) {
            if (x.leaf) {
                out.push_back(x.item[i]);
            } else {
                collect(*x.c[i], out);
            }
        }
    }

    // Checks the subtree rooted at x. Returns its height, or -1 on a violation.
    //   - Every node holds between t and 2t - 1 entries; the root at least 1 as a leaf, else 2.
    //   - A leaf's summaries are its items', and a nonleaf node's are its children's totals.
    //   - Child slots past the last entry are null, and every leaf sits at the same depth.
    static int check_subtree(const Node& x, bool is_root) {
        int min_entries = is_root ? (x.leaf ? 1 : 2) : t;
        if (x.n < min_entries || x.n > 2 * t - 1) return -1;
        if (x.leaf) {
            for (int i = 0; i < x.n; i++) {
                if (x.summary[i] != x.item[i].summary()) return -1;
            }
            return 0;
        }
        for (int i = x.n; i < 2 * t; i++) {
            if (x.c[i] != nullptr) return -1;
        }
        int height = -1;
        for (int i = 0; i < x.n; i++) {
            if (x.c[i] == nullptr || x.summary[i] != total(*x.c[i])) return -1;
            int child_height = check_subtree(*x.c[i], false);
            if (child_height < 0) return -1;
            if (i == 0) {
                height = child_height;
            } else if (child_height != height) {
                return -1;
            }
        }
        return height + 1;
    }

    std::unique_ptr<Node> root_;  // Null when the sequence is empty.
};

}  // namespace btree

// Prints the tree's shape, one node per line indented by depth, for std::println("{}", tree): a
// nonleaf node's entries are its children's summaries, a leaf's are its items, and both print
// through their own std::formatter. The empty tree prints as [].
template <btree::Measured Item, int t>
    requires std::formattable<Item, char> &&
             std::formattable<typename btree::SumTree<Item, t>::S, char>
struct std::formatter<btree::SumTree<Item, t>, char> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const btree::SumTree<Item, t>& tree, auto& ctx) const {
        if (tree.root() == nullptr) return std::format_to(ctx.out(), "[]");
        return write(*tree.root(), 0, ctx.out());
    }

private:
    using Node = typename btree::SumTree<Item, t>::Node;

    template <typename Out>
    static Out write(const Node& x, int depth, Out out) {
        if (depth > 0) *out++ = '\n';
        out = std::fill_n(out, 2 * depth, ' ');
        if (x.leaf) out = std::format_to(out, "leaf ");
        *out++ = '[';
        for (int i = 0; i < x.n; i++) {
            if (i > 0) *out++ = ' ';
            if (x.leaf) {
                out = std::format_to(out, "{}", x.item[i]);
            } else {
                out = std::format_to(out, "{}", x.summary[i]);
            }
        }
        *out++ = ']';
        if (!x.leaf) {
            for (int i = 0; i < x.n; i++) out = write(*x.c[i], depth + 1, out);
        }
        return out;
    }
};
