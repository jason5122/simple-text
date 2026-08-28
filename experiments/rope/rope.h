#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rope {

// The tree (Node and friends) is defined entirely in rope.cc, not declared
// here, so changing its shape -- e.g. adding a per-node metric -- only
// recompiles rope.cc, not every includer of this header. That's also why
// Rope's destructor and move operations are declared below but defined in
// the .cc: std::unique_ptr<Node> needs a complete Node at the point they're
// actually defined, and that's only true there.
struct Node;

// A leaf's contribution to the document's widest-line computation: the
// widest complete line fully inside it, plus the width of the still-open
// run at its very start/end (a line can span a leaf boundary, so "this
// leaf's first/last line" isn't a complete line until combined with its
// neighbors -- see combine()). newline_count here is local bookkeeping for
// that combine, not the same thing as the document-wide newline count
// Rope::line_count() reports.
//
// Unlike length/newline_count, a leaf's width can't be computed from the
// leaf's text alone without a font -- see Rope::LeafHandle below for how a
// real value gets in.
struct LineWidthMetric {
    double widest_complete_line = 0;
    double leading_open = 0;
    double trailing_open = 0;
    size_t newline_count = 0;

    // Combines two adjacent subtrees' metrics into their parent's -- see
    // rope.cc for the boundary-crossing logic.
    static LineWidthMetric combine(const LineWidthMetric& left, const LineWidthMetric& right);
};

// A leaf's contribution to visual-row counting at one registered wrap
// width (see Rope::WrapHandle). Deliberately reuses LineWidthMetric's
// exact shape rather than tracking wrap points directly: soft-wrap
// position is context-dependent (how much of a neighboring leaf's line
// carries in shifts where THIS leaf would wrap, unlike a '\n', which is
// absolute), so combining independently-measured per-leaf wrap positions
// can't be made exactly correct without re-deriving them from scratch --
// this rope has no font, so it can't. Reducing to "row count per complete
// *logical* line" sidesteps that entirely: leading_open_width/
// trailing_open_width/complete_rows mean exactly what they do in
// LineWidthMetric (widths of logical lines, not yet reduced to rows until
// a line's full width is known), and newline_count plays the same role in
// combine(). A row count only gets derived from a width via
// ceil(width / wrap_width) once, when a line's width is finalized (see
// rope.cc) -- matching how real word-wrap always resets per logical line,
// never carries state across a hard newline.
struct SoftWrapMetric {
    size_t complete_rows = 0;
    double leading_open_width = 0;
    double trailing_open_width = 0;
    size_t newline_count = 0;
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
    Rope();
    explicit Rope(std::string_view text);
    ~Rope();
    Rope(Rope&&) noexcept;
    Rope& operator=(Rope&&) noexcept;
    Rope(const Rope&) = delete;
    Rope& operator=(const Rope&) = delete;

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

    // Width of the widest measured line in the document -- leaves whose
    // width hasn't been pushed via set_leaf_width() (including leaves that
    // were just edited and are dirty again) contribute zero. Callers that
    // need an up-to-date answer should drain dirty_leaves() into
    // set_leaf_width() first. This is what a horizontal scrollbar's range
    // would be based on.
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

    // Opaque reference to one leaf, obtained from dirty_leaves(). Valid
    // only until the next insert()/erase() call, since edits can split,
    // merge, or otherwise invalidate leaves.
    class LeafHandle {
    public:
        bool operator==(const LeafHandle&) const = default;

    private:
        friend class Rope;
        explicit LeafHandle(Node* node) : node_(node) {}
        Node* node_ = nullptr;
    };

    // Leaves whose text changed since their width was last set via
    // set_leaf_width() (including leaves that have never been measured) --
    // mirrors Sublime Text's per-block dirty flag (see
    // reverse_engineering/node_types.md), so a layout pass only re-measures
    // what actually changed instead of the whole document after every edit.
    // This call itself is O(total leaves), not O(dirty leaves): it's a
    // linear scan of the leaf chain checking a flag, not (yet) a subtree
    // skip like Sublime Text's own dirty-flag-gated descent. Worth
    // revisiting if a profile ever shows it matters -- see
    // //experiments/rope:rope_benchmark.
    std::vector<LeafHandle> dirty_leaves() const;

    // The text of `leaf`, to feed to a real font/text-shaping backend.
    std::string_view leaf_text(LeafHandle leaf) const;

    // Records `leaf`'s measured width, clears its dirty flag, and
    // incrementally re-aggregates line_width from `leaf` up to the root --
    // not a full-tree walk, the same "only touch the dirty path" shape as
    // Sublime Text's updateChildExtents.
    void set_leaf_width(LeafHandle leaf, LineWidthMetric width);

    // Opaque reference to one registered wrap width, obtained from
    // register_wrap_width(). Stays valid until *that width's own*
    // unregister_wrap_width() call, regardless of what happens to other
    // widths -- internally this is a stable id, not the raw compact index
    // Sublime Text's TokenStorage::removeLayout shifts on removal (see
    // reverse_engineering/soft_wrap_caching.md); the shifting still happens
    // to the underlying storage, it's just not something callers have to
    // track.
    class WrapHandle {
    public:
        bool operator==(const WrapHandle&) const = default;

    private:
        friend class Rope;
        explicit WrapHandle(size_t id) : id_(id) {}
        size_t id_ = 0;
    };

    // Registers a new wrap width (e.g. for a newly opened view) and
    // returns a handle used to read/update its cached layout. Soft wrap is
    // inherently per-view -- two panes can wrap the same document
    // differently -- so unlike line_width this isn't a single value: every
    // node in the tree gets a new slot for it, mirroring Sublime Text's
    // TokenStorage::addLayout (see
    // reverse_engineering/soft_wrap_caching.md). O(total nodes).
    WrapHandle register_wrap_width(double width);

    // Deregisters a wrap width (e.g. a view closing), freeing its slot in
    // every node. O(total nodes): mirrors TokenStorage::removeLayout,
    // which walks the whole tree shifting every node's slot array to stay
    // compact -- see reverse_engineering/soft_wrap_caching.md. `handle` is
    // invalid after this call; other outstanding handles are unaffected.
    void unregister_wrap_width(WrapHandle handle);

    // Leaves whose wrap layout at `handle` is stale (never measured, or
    // edited since last measured) -- the wrap-width-specific counterpart
    // to dirty_leaves(), with the same O(total leaves) caveat.
    std::vector<LeafHandle> dirty_leaves_for_wrap(WrapHandle handle) const;

    // Records `leaf`'s measured row layout at `handle`'s wrap width,
    // clears that slot's dirty flag, and incrementally re-aggregates the
    // wrap layout from `leaf` up to the root -- the wrap-width-specific
    // counterpart to set_leaf_width().
    void set_leaf_wrap(LeafHandle leaf, WrapHandle handle, SoftWrapMetric metric);

    // Total visual rows in the document at `handle`'s wrap width -- at
    // least 1, even for an empty document (unlike widest_line_width(),
    // where 0 is a legitimate "no width" answer, a document always
    // occupies at least one, possibly empty, row). Same "only meaningful
    // once fully measured" contract as widest_line_width(): drain
    // dirty_leaves_for_wrap(handle) into set_leaf_wrap() first.
    size_t visual_row_count(WrapHandle handle) const;

    // Writes a self-contained HTML file that renders the tree as an
    // in-browser Graphviz graph (pannable/zoomable, no local `dot` install
    // needed -- just an internet connection to fetch it from a CDN). Solid
    // edges are the tree structure; dashed edges trace the leaf chain.
    void write_html(const std::string& path) const;

private:
    std::unique_ptr<Node> root_;
    // Registered wrap widths and their stable ids, index-parallel (and
    // index-parallel with every node's wrap_layouts/wrap_dirty). Kept
    // compact rather than tombstoned on removal, matching Sublime Text's
    // confirmed shift-compact TokenStorage::removeLayout behavior --
    // WrapHandle::id_ -> current index is a linear search over
    // wrap_ids_ (see rope.cc), which is fine given the number of
    // simultaneously registered widths is expected to be small (roughly
    // "how many views are open").
    std::vector<double> wrap_widths_;
    std::vector<size_t> wrap_ids_;
    size_t next_wrap_id_ = 0;
};

}  // namespace rope
