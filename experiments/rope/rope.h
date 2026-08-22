#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace rope {

// The tree (Node and friends) is defined entirely in rope.cc, not declared
// here, so changing its shape -- e.g. adding a per-node metric -- only
// recompiles rope.cc, not every includer of this header. That's also why
// Rope's destructor and move operations are declared below but defined in
// the .cc: std::unique_ptr<Node> needs a complete Node at the point they're
// actually defined, and that's only true there.
struct Node;

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

    // Writes a self-contained HTML file that renders the tree as an
    // in-browser Graphviz graph (pannable/zoomable, no local `dot` install
    // needed -- just an internet connection to fetch it from a CDN). Solid
    // edges are the tree structure; dashed edges trace the leaf chain.
    void write_html(const std::string& path) const;

private:
    std::unique_ptr<Node> root_;
};

}  // namespace rope
