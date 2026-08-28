#include "experiments/rope/rope.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fuzztest/fuzztest_core.h>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace rope {
namespace {

static_assert(std::movable<Rope>);

// Fake per-character width, playing the role a real font backend would
// (//experiments/rasterizer isn't wired up yet). Deliberately not uniform
// (some characters wider/narrower than others) so a "widest by width" test
// can differ from "longest by character count" and actually prove the
// measurement is width-based. Used below to drive Rope's push-based width
// API (measure_leaf_width()/remeasure_all()), playing the same role a real
// layout pass would -- Rope itself has no notion of character width at all
// anymore (see LineWidthMetric's comment in rope.h for why).
double reference_char_width(char c) {
    switch (c) {
    case '\t':
        return 4.0;
    case 'i':
    case 'l':
    case '.':
    case ',':
    case '\'':
        return 0.5;
    case 'm':
    case 'w':
    case 'M':
    case 'W':
        return 1.5;
    default:
        return 1.0;
    }
}

// Reference "widest line" computation: split on '\n', measure each piece
// with reference_char_width(), take the max. Independent of Rope's own
// LineWidthMetric::combine logic, so it's a genuine cross-check rather than
// exercising the same code path twice.
double widest_line_width_of(const std::string& s) {
    double widest = 0;
    size_t start = 0;
    while (true) {
        size_t end = s.find('\n', start);
        std::string_view piece = end == std::string::npos
                                     ? std::string_view(s).substr(start)
                                     : std::string_view(s).substr(start, end - start);
        double width = 0;
        for (char c : piece) {
            width += reference_char_width(c);
        }
        widest = std::max(widest, width);
        if (end == std::string::npos) {
            return widest;
        }
        start = end + 1;
    }
}

// Builds the LineWidthMetric a real layout pass would push via
// set_leaf_width() for one leaf's text -- the per-character loop that used
// to live inside Rope itself (as LineWidthMetric::of_leaf) before width
// measurement moved outside the rope.
LineWidthMetric measure_leaf_width(std::string_view text) {
    LineWidthMetric result;
    double current = 0;
    for (char c : text) {
        if (c == '\n') {
            result.widest_complete_line = std::max(result.widest_complete_line, current);
            if (result.newline_count == 0) {
                result.leading_open = current;
            }
            ++result.newline_count;
            current = 0;
        } else {
            current += reference_char_width(c);
        }
    }
    result.trailing_open = current;
    if (result.newline_count == 0) {
        result.leading_open = current;
    }
    return result;
}

// Measures every leaf whose width is stale and pushes the result in --
// what a real layout pass does after a batch of edits, before anything
// reads widest_line_width().
void remeasure_all(Rope& r) {
    for (Rope::LeafHandle leaf : r.dirty_leaves()) {
        r.set_leaf_width(leaf, measure_leaf_width(r.leaf_text(leaf)));
    }
}

// Rows a single logical line of `width` occupies once wrapped at
// `wrap_width` -- mirrors rope.cc's rows_for() exactly (duplicated rather
// than exposed, same reasoning as reference_char_width() mirroring
// fake_char_width()). Always at least 1, even for an empty line.
size_t rows_for(double width, double wrap_width) {
    return width > 0 ? static_cast<size_t>(std::ceil(width / wrap_width)) : size_t{1};
}

// Builds the SoftWrapMetric a real layout pass would push via
// set_leaf_wrap() for one leaf's text at `wrap_width`. Character-wrap, not
// word-wrap (wraps mid-word if that's where the width threshold falls) --
// a deliberate simplification (real word-wrap is a font/Unicode-shaping
// concern, out of scope here, same spirit as reference_char_width()
// standing in for a real font). Otherwise identical in shape to
// measure_leaf_width(): tracks logical-line widths exactly the same way,
// only converting a line's width to a row count once, when the line
// closes -- see SoftWrapMetric's comment in rope.h for why.
SoftWrapMetric measure_leaf_wrap(std::string_view text, double wrap_width) {
    SoftWrapMetric result;
    double current = 0;
    for (char c : text) {
        if (c == '\n') {
            if (result.newline_count == 0) {
                result.leading_open_width = current;
            } else {
                result.complete_rows += rows_for(current, wrap_width);
            }
            ++result.newline_count;
            current = 0;
        } else {
            current += reference_char_width(c);
        }
    }
    result.trailing_open_width = current;
    if (result.newline_count == 0) {
        result.leading_open_width = current;
    }
    return result;
}

// Measures every leaf whose wrap layout at `handle` is stale and pushes
// the result in, at `wrap_width` -- the wrap-width-specific counterpart to
// remeasure_all(). The caller has to supply `wrap_width` itself since
// Rope doesn't expose "what width did I register this handle for".
void remeasure_all_wrap(Rope& r, Rope::WrapHandle handle, double wrap_width) {
    for (Rope::LeafHandle leaf : r.dirty_leaves_for_wrap(handle)) {
        r.set_leaf_wrap(leaf, handle, measure_leaf_wrap(r.leaf_text(leaf), wrap_width));
    }
}

// Reference visual-row count: split on '\n' (exactly like
// widest_line_width_of()), measure each line's width, convert to a row
// count via rows_for(), sum -- independent of the tree/leaf machinery
// entirely, a genuine cross-check rather than the same code path exercised
// twice.
size_t total_visual_rows_of(const std::string& s, double wrap_width) {
    size_t rows = 0;
    size_t start = 0;
    while (true) {
        size_t end = s.find('\n', start);
        std::string_view piece = end == std::string::npos
                                     ? std::string_view(s).substr(start)
                                     : std::string_view(s).substr(start, end - start);
        double width = 0;
        for (char c : piece) {
            width += reference_char_width(c);
        }
        rows += rows_for(width, wrap_width);
        if (end == std::string::npos) {
            return rows;
        }
        start = end + 1;
    }
}

TEST(RopeTest, DefaultConstructedIsEmpty) {
    Rope r;
    EXPECT_EQ(r.str(), "");
}

TEST(RopeTest, ConstructFromShortString) {
    Rope r("hi");  // Shorter than one leaf.
    EXPECT_EQ(r.str(), "hi");
}

TEST(RopeTest, ConstructForcesMultipleSplits) {
    // Long enough that building it in one insert() call overflows a leaf
    // many times over, then overflows the internal node that leaf-splitting
    // produces -- the exact shape that exposed a real ordering bug in
    // split_overflowing_internal (it collected multi-way overflow siblings
    // out of left-to-right order).
    std::string text;
    for (int i = 0; i < 2500; ++i) {
        text += "The quick brown fox jumps over the lazy dog. ";
    }
    Rope r(text);
    EXPECT_EQ(r.str(), text);
}

TEST(RopeTest, InsertAtFrontMiddleAndEnd) {
    Rope r("The quick brown fox jumps over the lazy dog");

    r.insert(0, "[[");
    r.insert(r.size(), "]]");
    r.insert(r.size() / 2, " ***MIDDLE*** ");

    EXPECT_EQ(r.str(), "[[The quick brown fox j ***MIDDLE*** umps over the lazy dog]]");
}

TEST(RopeTest, InsertEmptyStringIsNoOp) {
    Rope r("hello");
    r.insert(2, "");
    EXPECT_EQ(r.str(), "hello");
}

TEST(RopeTest, InsertCausesMultiWayInternalOverflow) {
    // One insert() call producing far more new leaves than a single split
    // can absorb, forcing split_overflowing_internal to run several
    // iterations -- the same shape as the bug referenced above.
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);
    EXPECT_EQ(r.str(), text);
}

TEST(RopeTest, EraseFromFrontUntilEmpty) {
    Rope r("abc");
    r.erase(0, 1);
    EXPECT_EQ(r.str(), "bc");
    r.erase(0, 1);
    EXPECT_EQ(r.str(), "c");
    r.erase(0, 1);
    EXPECT_EQ(r.str(), "");
}

TEST(RopeTest, EraseEmptiesALeafAndRelinksChain) {
    // Two full leaves' worth of text (rope.h documents the leaf cap), as
    // two distinct runs so it's obvious where the leaf boundary was.
    // Erasing the first leaf entirely must repoint the second leaf to the
    // front of the chain rather than leaving str() walk into a freed leaf.
    std::string text(4096, 'A');
    text += std::string(4096, 'B');
    Rope r(text);

    r.erase(0, 4096);
    EXPECT_EQ(r.str(), std::string(4096, 'B'));
}

TEST(RopeTest, EraseSpanningMultipleLeavesAndInternalChildren) {
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);

    text.erase(5, 80000);
    r.erase(5, 80000);
    EXPECT_EQ(r.str(), text);
}

TEST(RopeTest, EraseEverythingEmptiesTheRope) {
    Rope r("hello world");
    r.erase(0, r.size());
    EXPECT_EQ(r.str(), "");
}

TEST(RopeTest, EraseZeroCountIsNoOp) {
    Rope r("hello");
    r.erase(2, 0);
    EXPECT_EQ(r.str(), "hello");
}

TEST(RopeTest, CharAtMatchesEveryPosition) {
    // char_at() descends the tree by cached length; str() just walks the
    // linked leaf chain. The two share almost no code, so this is checked
    // once here rather than folded into every test above.
    std::string text = "The quick brown fox jumps over the lazy dog";
    Rope r(text);
    for (size_t i = 0; i < text.size(); ++i) {
        EXPECT_EQ(r.char_at(i), text[i]) << "at position " << i;
    }
}

TEST(RopeTest, LineCountAndLineAt) {
    std::string text = "Line one\nLine two is longer\nShort\n\nLine five\nLast line";
    Rope r(text);
    EXPECT_EQ(r.line_count(), size_t{6});

    size_t line = 0;
    for (size_t i = 0; i <= text.size(); ++i) {
        EXPECT_EQ(r.line_at(i), line) << "at position " << i;
        if (i < text.size() && text[i] == '\n') {
            ++line;
        }
    }
}

TEST(RopeTest, LineContent) {
    Rope r("hello\nworld\nlast line");
    EXPECT_EQ(r.line_content(0), "hello");
    EXPECT_EQ(r.line_content(1), "world");
    EXPECT_EQ(r.line_content(2), "last line");
}

TEST(RopeTest, LineContentWithEmptyLines) {
    Rope r("a\n\nb");
    EXPECT_EQ(r.line_content(0), "a");
    EXPECT_EQ(r.line_content(1), "");
    EXPECT_EQ(r.line_content(2), "b");
}

TEST(RopeTest, WidestLineWidth) {
    // "hi" = 1 + 0.5 = 1.5; "www" = 1.5 * 3 = 4.5; "x" = 1.
    Rope r("hi\nwww\nx");
    remeasure_all(r);
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 4.5);
}

TEST(RopeTest, WidestLineWidthPrefersWidthOverCharacterCount) {
    // "wwww" is 4 characters but width 6; "iiiiiiii" is 8 characters (twice
    // as many) but width 4. If this measured length instead of width, it'd
    // pick the wrong line.
    Rope r("wwww\niiiiiiii");
    remeasure_all(r);
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 6.0);
}

TEST(RopeTest, WidestLineWidthSpansMultipleLeaves) {
    // A single line longer than one leaf (rope.h documents a 4096-char
    // cap), so computing its width requires LineWidthMetric::combine to
    // handle a line crossing a leaf boundary, not just per-leaf maxima.
    std::string wide_line(5000, 'a');  // 5000 * reference_char_width('a') == 5000.
    Rope r("short\n" + wide_line + "\nalso short");
    remeasure_all(r);
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 5000.0);
}

TEST(RopeTest, WidestLineWidthOfEmptyRopeIsZero) {
    Rope r;
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 0.0);
}

TEST(RopeTest, DirtyLeavesInitiallyCoverWholeDocument) {
    // Long enough to force several leaves (rope.h documents the 4096-char
    // cap), so this actually exercises more than one handle, not just the
    // trivial single-leaf case.
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);

    // Every leaf starts dirty (never measured), and dirty_leaves() returns
    // them in leaf-chain order, so concatenating their text should
    // reconstruct the whole document.
    std::string reconstructed;
    for (Rope::LeafHandle leaf : r.dirty_leaves()) {
        reconstructed += r.leaf_text(leaf);
    }
    EXPECT_EQ(reconstructed, text);
}

TEST(RopeTest, LeafTextMatchesLeafContent) {
    Rope r("hello");
    auto leaves = r.dirty_leaves();
    ASSERT_EQ(leaves.size(), size_t{1});
    EXPECT_EQ(r.leaf_text(leaves.front()), "hello");
}

TEST(RopeTest, SetLeafWidthClearsDirtyFlag) {
    Rope r("hello");
    ASSERT_EQ(r.dirty_leaves().size(), size_t{1});

    r.set_leaf_width(r.dirty_leaves().front(), LineWidthMetric{});
    EXPECT_TRUE(r.dirty_leaves().empty());
}

TEST(RopeTest, EditingALeafMarksItDirtyAgain) {
    Rope r("hello world");
    remeasure_all(r);
    ASSERT_TRUE(r.dirty_leaves().empty());

    r.insert(5, "!!!");
    EXPECT_EQ(r.dirty_leaves().size(), size_t{1});
}

TEST(RopeTest, PartiallyMeasuredTreeIsNotMeaningful) {
    // widest_line_width()'s contract (see rope.h) is "only meaningful once
    // dirty_leaves() is fully drained": an unmeasured leaf's default-zero
    // LineWidthMetric isn't a safe stand-in to combine with a real one, so
    // a partially-measured tree can give an answer that's neither correct
    // nor a clean underestimate. This pins that down as known, expected
    // behavior rather than leaving it undocumented.
    std::string wide_line(5000, 'a');  // Longer than one leaf (4096 cap).
    Rope r(wide_line + "\nshort");
    auto leaves = r.dirty_leaves();
    ASSERT_GT(leaves.size(), size_t{1});

    r.set_leaf_width(leaves.front(), measure_leaf_width(r.leaf_text(leaves.front())));
    // Not 5000: the still-dirty second leaf's default-zero leading_open
    // gets folded into the first leaf's real trailing_open as if it were a
    // genuine (empty) boundary contribution.
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 4096.0);
}

TEST(RopeTest, VisualRowCountOfEmptyRopeIsOne) {
    // Unlike widest_line_width() (0 is a legitimate "no width" answer for
    // an empty document), an empty document still occupies exactly one
    // (empty) row -- the fuzzer caught this returning 0 before the fix.
    Rope r;
    Rope::WrapHandle handle = r.register_wrap_width(80);
    EXPECT_EQ(r.visual_row_count(handle), size_t{1});
}

TEST(RopeTest, VisualRowCountNoWrapNeeded) {
    Rope r("hello");
    Rope::WrapHandle handle = r.register_wrap_width(1000);
    remeasure_all_wrap(r, handle, 1000);
    EXPECT_EQ(r.visual_row_count(handle), size_t{1});
}

TEST(RopeTest, VisualRowCountWithHardNewlinesOnly) {
    // Wrap width huge enough that only '\n' -- never the width threshold
    // -- can trigger a row break.
    Rope r("a\nb\nc");
    Rope::WrapHandle handle = r.register_wrap_width(1000);
    remeasure_all_wrap(r, handle, 1000);
    EXPECT_EQ(r.visual_row_count(handle), size_t{3});
}

TEST(RopeTest, VisualRowCountWithSoftWrapOnly) {
    // 7 'a's (width 1 each) wrapped at 3: "aaa" | "aaa" | "a" -- 3 rows,
    // no '\n' anywhere.
    Rope r("aaaaaaa");
    Rope::WrapHandle handle = r.register_wrap_width(3);
    remeasure_all_wrap(r, handle, 3);
    EXPECT_EQ(r.visual_row_count(handle), size_t{3});
}

TEST(RopeTest, VisualRowCountSpansMultipleLeaves) {
    // A single unbroken run longer than one leaf (rope.h documents the
    // 4096-char cap), so this requires combine_wrap() to handle a row
    // crossing a leaf boundary, not just per-leaf sums.
    std::string text(9000, 'a');
    Rope r(text);
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);
    EXPECT_EQ(r.visual_row_count(handle), total_visual_rows_of(text, 80));
}

TEST(RopeTest, MultipleSimultaneousWrapWidthsGiveIndependentAnswers) {
    // The actual point of the whole per-node growable-array design: two
    // "views" of the same document, wrapping at different widths, get
    // different (both correct) answers from the same underlying tree.
    std::string text(200, 'a');
    Rope r(text);
    Rope::WrapHandle narrow = r.register_wrap_width(40);
    Rope::WrapHandle wide = r.register_wrap_width(100);

    remeasure_all_wrap(r, narrow, 40);
    remeasure_all_wrap(r, wide, 100);

    EXPECT_EQ(r.visual_row_count(narrow), total_visual_rows_of(text, 40));
    EXPECT_EQ(r.visual_row_count(wide), total_visual_rows_of(text, 100));
    EXPECT_NE(r.visual_row_count(narrow), r.visual_row_count(wide));
}

TEST(RopeTest, DirtyLeavesForWrapInitiallyCoverWholeDocument) {
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);
    Rope::WrapHandle handle = r.register_wrap_width(80);

    std::string reconstructed;
    for (Rope::LeafHandle leaf : r.dirty_leaves_for_wrap(handle)) {
        reconstructed += r.leaf_text(leaf);
    }
    EXPECT_EQ(reconstructed, text);
}

TEST(RopeTest, RegisteringAfterContentExistsMarksEverythingDirtyForIt) {
    Rope r("hello world");
    Rope::WrapHandle handle = r.register_wrap_width(80);
    // Registered after the rope already had content -- every existing
    // leaf needs its first measurement at this width.
    EXPECT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});
}

TEST(RopeTest, SetLeafWrapClearsDirtyFlag) {
    Rope r("hello");
    Rope::WrapHandle handle = r.register_wrap_width(80);
    ASSERT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});

    r.set_leaf_wrap(r.dirty_leaves_for_wrap(handle).front(), handle, SoftWrapMetric{});
    EXPECT_TRUE(r.dirty_leaves_for_wrap(handle).empty());
}

TEST(RopeTest, EditingALeafMarksItWrapDirtyAgain) {
    Rope r("hello world");
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);
    ASSERT_TRUE(r.dirty_leaves_for_wrap(handle).empty());

    r.insert(5, "!!!");
    EXPECT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});
}

TEST(RopeTest, WrapAndWidthDirtyTrackingAreIndependent) {
    // One edit invalidates both caches, but clearing one doesn't clear the
    // other -- they're independent per-slot flags, not one bit for
    // everything (see Node::wrap_dirty's comment in rope.cc).
    Rope r("hello world");
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all(r);
    remeasure_all_wrap(r, handle, 80);
    r.insert(5, "!!!");
    ASSERT_EQ(r.dirty_leaves().size(), size_t{1});
    ASSERT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});

    remeasure_all(r);  // Clears line_width's dirty flag only.
    EXPECT_TRUE(r.dirty_leaves().empty());
    EXPECT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});
}

TEST(RopeTest, UnregisterDoesNotInvalidateOtherHandles) {
    // The whole point of WrapHandle's stable id (rope.h): unregistering an
    // earlier-registered width must not disturb a still-live handle for a
    // later one, even though the underlying storage shifts down to stay
    // compact (see reverse_engineering/soft_wrap_caching.md).
    std::string text(200, 'a');
    Rope r(text);
    Rope::WrapHandle first = r.register_wrap_width(40);
    Rope::WrapHandle second = r.register_wrap_width(100);
    remeasure_all_wrap(r, second, 100);

    r.unregister_wrap_width(first);

    EXPECT_EQ(r.visual_row_count(second), total_visual_rows_of(text, 100));
    EXPECT_TRUE(r.dirty_leaves_for_wrap(second).empty());
}

TEST(RopeTest, UnregisterMiddleHandlePreservesOthers) {
    // Three handles, removing the middle one -- unlike
    // UnregisterDoesNotInvalidateOtherHandles above (which only removes
    // the very first), this shifts the third handle's underlying index by
    // one without touching the first's at all, exercising both cases at
    // once.
    std::string text(200, 'a');
    Rope r(text);
    Rope::WrapHandle first = r.register_wrap_width(20);
    Rope::WrapHandle second = r.register_wrap_width(40);
    Rope::WrapHandle third = r.register_wrap_width(100);
    remeasure_all_wrap(r, first, 20);
    remeasure_all_wrap(r, third, 100);

    r.unregister_wrap_width(second);

    EXPECT_EQ(r.visual_row_count(first), total_visual_rows_of(text, 20));
    EXPECT_EQ(r.visual_row_count(third), total_visual_rows_of(text, 100));
}

TEST(RopeTest, RegisteringSameWidthTwiceGivesIndependentHandles) {
    // A width value isn't a dedup key -- two registrations of the same
    // width get two independent handles with independent dirty state.
    Rope r("hello");
    Rope::WrapHandle first = r.register_wrap_width(80);
    Rope::WrapHandle second = r.register_wrap_width(80);
    EXPECT_NE(first, second);

    r.set_leaf_wrap(r.dirty_leaves_for_wrap(first).front(), first, SoftWrapMetric{});
    EXPECT_TRUE(r.dirty_leaves_for_wrap(first).empty());
    EXPECT_FALSE(r.dirty_leaves_for_wrap(second).empty());
}

TEST(RopeTest, RegisterOnEmptyRopeThenInsertGetsCorrectSlots) {
    // register_wrap_width() on an empty rope has no nodes to touch (its
    // for_each_node walk is skipped) -- the first leaf, created later by
    // insert(), has to pick up the registered width on its own, via
    // make_leaf(wrap_widths_.size()) rather than starting with zero slots.
    Rope r;
    Rope::WrapHandle handle = r.register_wrap_width(80);
    r.insert(0, "hello");

    EXPECT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});
    remeasure_all_wrap(r, handle, 80);
    EXPECT_EQ(r.visual_row_count(handle), size_t{1});
}

TEST(RopeTest, LeafSplitDuringInsertGetsCorrectWrapSlots) {
    // Forces a leaf to overflow (rope.h documents the 4096-char cap)
    // while two wrap widths are registered -- the newly split-off leaf
    // must get correctly-sized wrap_layouts/wrap_dirty from
    // insert_into_leaf's own make_leaf() call, not just the leaves that
    // existed at registration time.
    Rope r(std::string(4090, 'a'));
    Rope::WrapHandle a = r.register_wrap_width(40);
    Rope::WrapHandle b = r.register_wrap_width(100);
    remeasure_all_wrap(r, a, 40);
    remeasure_all_wrap(r, b, 100);

    r.insert(r.size(), std::string(20, 'b'));  // Overflows the single leaf.
    EXPECT_EQ(r.dirty_leaves_for_wrap(a).size(), size_t{2});
    EXPECT_EQ(r.dirty_leaves_for_wrap(b).size(), size_t{2});

    std::string text(4090, 'a');
    text += std::string(20, 'b');
    remeasure_all_wrap(r, a, 40);
    remeasure_all_wrap(r, b, 100);
    EXPECT_EQ(r.visual_row_count(a), total_visual_rows_of(text, 40));
    EXPECT_EQ(r.visual_row_count(b), total_visual_rows_of(text, 100));
}

TEST(RopeTest, InternalNodeSplitPreservesWrapAggregation) {
    // Forces split_overflowing_internal (not just leaf splits) while a
    // wrap width is registered -- the newly created internal siblings'
    // wrap_layouts have to come from combine_wrap_children() over their
    // own children, via recompute_metadata(), not stay default-zero.
    std::string text;
    for (int i = 0; i < 15000; ++i) {
        text += std::format("word{} ", i);
    }
    Rope r(text);
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);
    EXPECT_EQ(r.visual_row_count(handle), total_visual_rows_of(text, 80));
}

TEST(RopeTest, EraseRemovingALeafExcludesItFromDirtyTracking) {
    std::string text(4096, 'A');
    text += std::string(4096, 'B');
    Rope r(text);
    Rope::WrapHandle handle = r.register_wrap_width(80);
    ASSERT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{2});

    r.erase(0, 4096);  // Removes the first leaf entirely.

    EXPECT_EQ(r.dirty_leaves_for_wrap(handle).size(), size_t{1});
    EXPECT_EQ(r.leaf_text(r.dirty_leaves_for_wrap(handle).front()), std::string(4096, 'B'));
}

TEST(RopeTest, EraseEntireDocumentThenWrapQueriesStillWork) {
    Rope r("hello world");
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);

    r.erase(0, r.size());

    EXPECT_TRUE(r.dirty_leaves_for_wrap(handle).empty());
    EXPECT_EQ(r.visual_row_count(handle), size_t{1});  // Empty doc, one empty row.
}

TEST(RopeTest, WidestLineWidthLineEndsExactlyAtLeafBoundary) {
    // The '\n' lands exactly on the leaf split point (rope.h documents
    // the 4096-char cap), so the first leaf's own trailing_open is 0 and
    // combine() sees an already-closed boundary -- a "clean split" edge
    // case distinct from WidestLineWidthSpansMultipleLeaves, which
    // deliberately splits mid-line.
    std::string text(4095, 'a');
    text += '\n';  // Exactly 4096 chars: the whole first leaf.
    text += std::string(50, 'b');
    Rope r(text);
    remeasure_all(r);
    EXPECT_DOUBLE_EQ(r.widest_line_width(), 4095.0);
}

TEST(RopeTest, LineWidthExactlyAtWrapWidthIsOneRow) {
    // A line whose width lands exactly on the wrap threshold must not
    // wrap -- ceil(80/80) == 1, not 2. Exactly the class of off-by-one
    // combine_wrap() got wrong before VisualRowCountSpansMultipleLeaves
    // caught it (see SoftWrapMetric's comment in rope.h).
    Rope r(std::string(80, 'a'));  // Width == 80, wrap_width == 80.
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);
    EXPECT_EQ(r.visual_row_count(handle), size_t{1});
}

TEST(RopeTest, LineWidthOneOverWrapWidthIsTwoRows) {
    Rope r(std::string(81, 'a'));  // Width == 81, one over the threshold.
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);
    EXPECT_EQ(r.visual_row_count(handle), size_t{2});
}

TEST(RopeTest, EmptyLogicalLineCountsAsOneRow) {
    // "a", "", "b" -- three logical lines, the middle one empty. An empty
    // line is still a real, renderable row, not zero rows.
    Rope r("a\n\nb");
    Rope::WrapHandle handle = r.register_wrap_width(80);
    remeasure_all_wrap(r, handle, 80);
    EXPECT_EQ(r.visual_row_count(handle), size_t{3});
}

TEST(RopeTest, VisualRowCountWithVeryLargeWrapWidthEqualsLineCount) {
    std::string text = "one\ntwo\nthree\nfour";
    Rope r(text);
    Rope::WrapHandle handle = r.register_wrap_width(1'000'000);
    remeasure_all_wrap(r, handle, 1'000'000);
    EXPECT_EQ(r.visual_row_count(handle), r.line_count());
}

TEST(RopeTest, VisualRowCountWithVerySmallWrapWidth) {
    // Wrap width smaller than a single character's width -- every
    // character effectively gets its own row. Sanity check this doesn't
    // hang or crash (the ceil()-based model has no per-character loop at
    // combine time, unlike the buggy modulo-loop version it replaced) and
    // still matches the reference.
    std::string text = "hello world";
    Rope r(text);
    Rope::WrapHandle handle = r.register_wrap_width(0.1);
    remeasure_all_wrap(r, handle, 0.1);
    EXPECT_EQ(r.visual_row_count(handle), total_visual_rows_of(text, 0.1));
}

// ---- Property-based (FuzzTest) differential tests against std::string ----
// These hold a plain std::string as the reference model, apply the same
// operations to it and to the Rope, and assert the two stay in sync. The
// fuzzer mutates toward interesting inputs (empty strings, embedded '\n',
// boundary offsets), covering far more states than the fixed cases above.
// Run one continuously with e.g. `unit_tests --fuzz=RopeFuzzTest.EditsMatchStringModel`.

void ConstructionMatchesInput(const std::string& s) {
    Rope r(s);
    EXPECT_EQ(r.str(), s);
}
FUZZ_TEST(RopeFuzzTest, ConstructionMatchesInput);

void CharAtMatchesString(const std::string& s) {
    Rope r(s);
    for (size_t i = 0; i < s.size(); ++i) {
        ASSERT_EQ(r.char_at(i), s[i]);
    }
}
FUZZ_TEST(RopeFuzzTest, CharAtMatchesString);

// An arbitrary sequence of inserts and erases keeps the rope equal to the
// same edits applied to a std::string. Each edit is (is_insert, offset,
// count, text); offsets/counts are taken modulo the current length so
// they're always valid for both.
void EditsMatchStringModel(
    const std::string& initial,
    const std::vector<std::tuple<bool, size_t, size_t, std::string>>& edits) {
    std::string str = initial;
    Rope r(initial);
    // Registered once, up front, so every edit below exercises the wrap
    // side's dirty-marking/slot-sizing/up-walk too (insert_into_leaf's and
    // split_overflowing_internal's own make_leaf()/make_internal() calls,
    // erase_from's mark_leaf_dirty(), recompute_metadata()'s wrap combine
    // after a structural children change) -- not just line_width's, and
    // not just at single-shot construction like
    // VisualRowCountMatchesManualSplit below does.
    Rope::WrapHandle wrap = r.register_wrap_width(37);

    for (const auto& [is_insert, raw_pos, raw_count, text] : edits) {
        if (is_insert) {
            size_t pos = raw_pos % (str.size() + 1);
            str.insert(pos, text);
            r.insert(pos, text);
        } else {
            if (str.empty()) {
                continue;
            }
            size_t pos = raw_pos % str.size();
            size_t count = raw_count % (str.size() - pos + 1);
            str.erase(pos, count);
            r.erase(pos, count);
        }
        ASSERT_EQ(r.str(), str);
        // Exercises insert_into_leaf's/erase_from's dirty-marking and
        // set_leaf_width()'s up-walk on every single edit (unlike
        // WidestLineWidthMatchesManualSplit below, which only ever
        // constructs a rope in one shot and remeasures once).
        remeasure_all(r);
        ASSERT_TRUE(r.dirty_leaves().empty());
        ASSERT_DOUBLE_EQ(r.widest_line_width(), widest_line_width_of(str));

        remeasure_all_wrap(r, wrap, 37);
        ASSERT_TRUE(r.dirty_leaves_for_wrap(wrap).empty());
        ASSERT_EQ(r.visual_row_count(wrap), total_visual_rows_of(str, 37));
    }
}
FUZZ_TEST(RopeFuzzTest, EditsMatchStringModel);

// line_at agrees with a direct scan of the text at every position, and
// line_count matches the total number of '\n' bytes plus one.
void LineAtMatchesScan(const std::string& s) {
    Rope r(s);
    size_t line = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        ASSERT_EQ(r.line_at(i), line);
        if (i < s.size() && s[i] == '\n') {
            ++line;
        }
    }
    EXPECT_EQ(r.line_count(), line + 1);
}
FUZZ_TEST(RopeFuzzTest, LineAtMatchesScan);

// line_content agrees with manually splitting the text on '\n'.
void LineContentMatchesManualSplit(const std::string& s) {
    Rope r(s);
    size_t start = 0;
    for (size_t line = 0; line < r.line_count(); ++line) {
        size_t end = s.find('\n', start);
        std::string expected =
            end == std::string::npos ? s.substr(start) : s.substr(start, end - start);
        ASSERT_EQ(r.line_content(line), expected);
        start = end == std::string::npos ? s.size() : end + 1;
    }
}
FUZZ_TEST(RopeFuzzTest, LineContentMatchesManualSplit);

// widest_line_width agrees with manually splitting the text on '\n' and
// measuring each line with the same (fake) per-character width function.
void WidestLineWidthMatchesManualSplit(const std::string& s) {
    Rope r(s);
    remeasure_all(r);
    ASSERT_DOUBLE_EQ(r.widest_line_width(), widest_line_width_of(s));
}
FUZZ_TEST(RopeFuzzTest, WidestLineWidthMatchesManualSplit);

// visual_row_count agrees with manually splitting the text on '\n' and
// converting each line's width to a row count via rows_for() -- the same
// cross-check as WidestLineWidthMatchesManualSplit, but exercising
// combine_wrap()'s boundary-crossing logic specifically (this is what
// caught VisualRowCountSpansMultipleLeaves' original bug: independently-
// measured leaf summaries can't just be joined by summing widths without
// re-deriving wrap positions -- see SoftWrapMetric's comment in rope.h).
// raw_wrap_width is reduced to a small positive range the same way
// EditsMatchStringModel reduces its offsets -- FUZZ_TEST would otherwise
// happily try wrap_width == 0.
void VisualRowCountMatchesManualSplit(const std::string& s, uint32_t raw_wrap_width) {
    double wrap_width = 1 + (raw_wrap_width % 200);
    Rope r(s);
    Rope::WrapHandle handle = r.register_wrap_width(wrap_width);
    remeasure_all_wrap(r, handle, wrap_width);
    ASSERT_EQ(r.visual_row_count(handle), total_visual_rows_of(s, wrap_width));
}
FUZZ_TEST(RopeFuzzTest, VisualRowCountMatchesManualSplit);

// Registers several wrap widths, then unregisters them in a fuzzed order,
// checking every still-live handle after each removal. Stress-tests
// WrapHandle's stable-id indirection (rope.h) against arbitrary
// unregistration orderings -- not just the fixed "remove the first" /
// "remove the middle" shapes UnregisterDoesNotInvalidateOtherHandles and
// UnregisterMiddleHandlePreservesOthers above hand-pick.
void WrapHandleLifecycleSurvivesArbitraryUnregisterOrder(
    const std::string& s, const std::vector<uint8_t>& unregister_order) {
    Rope r(s);
    std::vector<std::pair<Rope::WrapHandle, double>> live;
    for (int i = 0; i < 5; ++i) {
        double width = 10.0 + i * 15;  // Distinct, reasonable widths: 10,25,40,55,70.
        Rope::WrapHandle handle = r.register_wrap_width(width);
        remeasure_all_wrap(r, handle, width);
        live.emplace_back(handle, width);
    }

    for (uint8_t raw_index : unregister_order) {
        if (live.empty()) {
            break;
        }
        size_t index = raw_index % live.size();
        r.unregister_wrap_width(live[index].first);
        live.erase(live.begin() + index);

        for (const auto& [handle, width] : live) {
            ASSERT_EQ(r.visual_row_count(handle), total_visual_rows_of(s, width));
        }
    }
}
FUZZ_TEST(RopeFuzzTest, WrapHandleLifecycleSurvivesArbitraryUnregisterOrder);

}  // namespace
}  // namespace rope
