#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rope {

// Shape shared by any per-node tree aggregate that isn't a plain sum: a
// leaf-local base case, plus a rule for combining two adjacent subtrees'
// values into their parent's. Rope::LineWidthMetric is the first instance;
// a future visual-row-count for soft wrap would be the second, and
// Rope::combine_children() is what lets it reuse this same shape instead of
// another hand-written fold. A concept, not a class member, since concepts
// can only be declared at namespace scope.
template <typename M>
concept Metric = requires(std::string_view text, const M& left, const M& right) {
    { M::of_leaf(text) } -> std::same_as<M>;
    { M::combine(left, right) } -> std::same_as<M>;
};

// A basic rope: a tree of string chunks, structured like the B+ tree in
// //experiments/b_tree. Internal nodes don't store keys -- they don't need
// to, since "where does index i live" is answered by walking child subtree
// lengths instead of comparing keys. Leaves hold text chunks and are linked
// left to right, same as B+ tree leaves, which makes reconstructing the
// full string a linear walk instead of a tree traversal.
//
// Leaves hold up to 4096 characters (one memory page -- also what Sublime
// Text's TokenBuffer buckets use, per a look at its shipped binary);
// internal nodes have at most 8 children (chosen empirically via
// //experiments/rope:rope_benchmark -- larger leaves cut leaf count and
// str()/char_at() cost substantially, but pushing branching factor past
// ~8 stopped helping: the larger per-node linear scan cost more than the
// shallower tree saved).
class Rope {
public:
    Rope() = default;
    explicit Rope(std::string_view text);

    size_t size() const;
    char char_at(size_t pos) const;

    // Lines are delimited by bare '\n', matching editor::PieceTree's
    // convention -- '\r' (from CRLF line endings) is just an ordinary
    // character that happens to sit at the end of a line's content.
    size_t line_count() const;

    // 0-indexed line containing `pos` (a leaf split can land in the middle
    // of any line, so this walks the tree counting '\n's passed along the
    // way rather than doing arithmetic).
    size_t line_at(size_t pos) const;

    // Content of 0-indexed line `line`, excluding its trailing '\n'.
    std::string line_content(size_t line) const;

    // Width of the widest line in the document, in the same units as
    // fake_char_width() (see rope.cc -- there's no real font backend wired
    // up yet, //experiments/rasterizer isn't ready for that, so widths are
    // a placeholder). This is what a horizontal scrollbar's range would be
    // based on.
    double widest_line_width() const;

    // Reconstructs the full string by walking the leaf chain left to right,
    // rather than recursing through the tree -- the payoff of linking leaves.
    std::string str() const;

    void insert(size_t pos, std::string_view text);

    // Removes the `count` characters starting at `pos`. A leaf emptied by
    // this is unlinked from the leaf chain and dropped from its parent;
    // an internal node emptied the same way propagates that removal up.
    // Underflowing nodes are not merged with a sibling afterward (unlike
    // overflow, underflow doesn't break correctness, just leaves the tree
    // less balanced than an ideal B+ tree would be).
    void erase(size_t pos, size_t count);

    // Renders the tree as Graphviz DOT source: solid edges for the tree
    // structure, dashed edges tracing the leaf chain.
    std::string to_dot() const;

    // Writes a self-contained HTML file that renders to_dot() in-browser
    // via a WASM build of Graphviz (pannable/zoomable, no local `dot`
    // install needed -- just an internet connection to fetch it from a CDN).
    void write_html(const std::string& path) const;

private:
    static constexpr size_t kMaxLeafLen = 4096;
    static constexpr size_t kOrder = 8;

    // Unlike length/newline_count, a line's width can't be reduced to a
    // single size_t summed by descend<Nav, Acc>, because a line can span a
    // leaf boundary. A node caches:
    //   - widest_complete_line: the widest line fully contained in this
    //     subtree (both ends closed off by a '\n' somewhere in the tree).
    //   - leading_open / trailing_open: the width of the still-unclosed
    //     run at the very start / end of this subtree's text.
    // Combining two children has to consider a line that spans the
    // boundary between them (left's trailing_open + right's leading_open)
    // as a candidate for the widest line -- the same "does the maximum
    // subarray cross the midpoint" case a segment tree has to handle,
    // just applied to a B-tree instead of a flat array.
    struct LineWidthMetric {
        double widest_complete_line = 0;
        double leading_open = 0;
        double trailing_open = 0;
        size_t newline_count = 0;

        // Base case: measures a leaf's own text directly.
        static LineWidthMetric of_leaf(std::string_view text);

        // Combines two adjacent subtrees' metrics into their parent's.
        static LineWidthMetric combine(const LineWidthMetric& left, const LineWidthMetric& right);
    };
    static_assert(Metric<LineWidthMetric>);

    struct Node {
        bool is_leaf;
        size_t length = 0;
        size_t newline_count = 0;
        LineWidthMetric line_width;

        explicit Node(bool leaf) : is_leaf(leaf) {}
        virtual ~Node() = default;
    };

    struct LeafNode : Node {
        std::string text;
        LeafNode* next = nullptr;

        LeafNode() : Node(true) {}
    };

    struct InternalNode : Node {
        std::vector<std::unique_ptr<Node>> children;

        InternalNode() : Node(false) {}
    };

    std::unique_ptr<Node> root_;

    static void recompute_metadata(Node* node);

    // Folds a Metric field across `children` left to right via M::combine,
    // starting from the first child's own value. The generic counterpart to
    // LineWidthMetric::combine's use in recompute_metadata: any future
    // Metric field just calls this the same way, rather than another
    // hand-written first/combine loop.
    template <Metric M, M Node::* Field>
    static M combine_children(const std::vector<std::unique_ptr<Node>>& children);

    // Placeholder for a real font/text-shaping backend (Core Text,
    // DirectWrite, Pango, ...) -- //experiments/rasterizer isn't wired up
    // yet. Sublime Text has the same kind of stand-in (MockUnicodeFont,
    // NullFont) for building/testing layout logic before a real backend
    // exists. Deliberately not uniform (some characters wider/narrower
    // than others) so a "widest by width" test can differ from "longest
    // by character count" and actually prove the metric measures width.
    static double fake_char_width(char c);

    // Every "find the leaf for X" query in this class -- char_at,
    // find_leaf_and_offset, line_at, line_start_offset -- is the same walk
    // with two things swapped: which cached field on Node decides which
    // child to descend into (`Nav`), and which field (if any) to sum over
    // every skipped sibling along the way (`Acc`). Sublime Text's
    // TokenStorage has a near-identical generic summarise<Metric>() for
    // this same reason: it's what lets a new per-node metric (e.g. a
    // future visual-row-count for soft wrap) get a working query for free
    // instead of another hand-written tree walk.
    //
    // `strict` distinguishes indexing an actual element (char_at: pos must
    // land strictly inside a child, via <) from a boundary/insertion-point
    // position (insert, line_at: pos may equal a child's cumulative Nav
    // exactly and still prefer that earlier child, via <=) -- see
    // find_leaf_and_offset's old standalone comment, preserved in the .cc,
    // for why the boundary case matters.
    //
    // On return, `pos` holds the leaf-local remainder and `accumulated`
    // holds the sum of `Acc` over every subtree skipped to get there.
    template <size_t Node::* Nav, size_t Node::* Acc = nullptr>
    static const LeafNode* descend(const Node* node,
                                   size_t& pos,
                                   bool strict,
                                   size_t& accumulated);

    // Returns any new right-hand siblings produced by a split, empty if none.
    std::vector<std::unique_ptr<Node>> insert_into(Node* node, size_t pos, std::string_view text);
    std::vector<std::unique_ptr<Node>> insert_into_leaf(LeafNode* leaf,
                                                        size_t pos,
                                                        std::string_view text);
    std::vector<std::unique_ptr<Node>> insert_into_internal(InternalNode* internal,
                                                            size_t pos,
                                                            std::string_view text);

    // Splits `internal`'s children into ceil(count / kOrder) balanced
    // groups when it has more than kOrder, since a large inserted string
    // can overflow it by more than one group's worth at a time. Groups
    // must be balanced, not peeled off in fixed kOrder-sized chunks: with
    // fixed-size peeling, the common one-over-kOrder overflow leaves
    // `internal` with just 1 child, and since inserts at the same edge
    // (e.g. repeated appends) keep overflowing that same node, the
    // imbalance compounds into a pathologically deep spine.
    std::vector<std::unique_ptr<Node>> split_overflowing_internal(InternalNode* internal);

    // Leaf containing `pos` (must be < size()), plus the offset of `pos`
    // within its text -- shared by char_at() and leaf_at().
    const LeafNode* leaf_containing(size_t pos, size_t& local_pos) const;

    // Non-const wrapper around leaf_containing(), for patching a leaf's
    // `next` pointer during erase.
    LeafNode* leaf_at(size_t pos);

    // Same kind of descent as leaf_containing(), but using <= instead of
    // strict < so pos == size() (a valid "one past the end" position, e.g.
    // a trailing empty line) still lands somewhere rather than silently
    // failing to descend. Used by line_content() to walk the leaf chain
    // instead of calling char_at() once per character.
    const LeafNode* find_leaf_and_offset(size_t pos, size_t& local_pos) const;

    // Absolute offset where `line` begins: the mirror image of line_at's
    // descent, walking by newline_count instead of length, then scanning
    // the target leaf for the position right after its Nth '\n'.
    size_t line_start_offset(size_t line) const;

    // Erases the local range [pos, pos + count) from `node`'s subtree.
    // `left_of_range` is the leaf immediately before the erased range (or
    // null if the range starts at position 0); it's threaded through by
    // reference so that when an erased leaf empties out, its predecessor
    // can be repointed past it, and so the predecessor can be advanced
    // when a leaf survives. Returns true if `node` is now empty and should
    // be removed by its caller.
    bool erase_from(Node* node, size_t pos, size_t count, LeafNode*& left_of_range);

    static std::string node_id(const Node* node);
    static std::string escape_dot_label(std::string_view text);
    void append_dot_nodes(std::string& dot, const Node* node) const;
    void append_dot_edges(std::string& dot, const Node* node) const;
};

}  // namespace rope
