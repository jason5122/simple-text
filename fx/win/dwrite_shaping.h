#pragma once

#include "fx/fx.h"
#include <cstddef>
#include <vector>
#include <windows.h>
// clang-format off: windows.h supplies the GDI types dwrite_3.h uses in its bitmap render target.
#include <dwrite_3.h>
// clang-format on

// Shaping arithmetic that belongs to us rather than to DirectWrite. Internal to //fx, and exposed
// for the same reason as fx_internal.h: this is pure, ours, and worth fuzzing directly.

// Fills in each glyph's source byte offset for one run.
//
// This is the step Core Text hands over for free: CTRunGetStringIndices already runs glyph ->
// text. DirectWrite only supplies the opposite direction, so the map has to be inverted. Trailing
// surrogates are skipped so a code point reports the offset of its lead unit, and the first offset
// of a cluster is then carried across any further glyphs it decomposed into, which is what keeps a
// ligature's components all pointing at the cluster's first byte.
//
// `indices_map` maps a UTF-16 offset in the shaped string to a byte offset in the UTF-8 original.
// A null or incomplete `desc` leaves every cluster at whatever the caller set. Every cluster this
// writes is a value read out of `indices_map`, so it can only ever name a byte the caller supplied.
void assign_clusters(const DWRITE_GLYPH_RUN_DESCRIPTION* desc,
                     const std::vector<size_t>& indices_map,
                     std::vector<fx_glyph>& glyphs);
