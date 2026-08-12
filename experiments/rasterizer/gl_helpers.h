#pragma once

#include "base/apple/scoped_cgtyperef.h"

// Displays the already-composited Core Graphics bitmap via a minimal OpenGL path (texture + quad),
// alongside the Core Graphics/NSImageView path in mac_helpers.h. This mirrors how Sublime Text
// (and our v1 //gui stack) present through GL, which composites wide-gamut color glyphs
// differently than the color-managed NSImageView path. Selected at runtime; see main.mm.
void show_window_gl(base::apple::ScopedCGContext ctx, double scale);
