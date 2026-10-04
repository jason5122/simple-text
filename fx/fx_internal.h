#pragma once

#include "base/geometry.h"
#include "fx/fx.h"
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

// The platform-independent pixel math behind fx_glyph_cache, exposed so it can be tested and
// fuzzed directly, and the seam between fx.cc and the platform backends. Internal to //fx: fx.h is
// the library's only public header, and nothing outside the library should include this.

// What each backend implements behind fx_create_font and fx_create_font_from_file. fx.cc wraps
// these with the checks every platform needs: a usable size going in, and finite, positive
// vertical metrics coming out. A backend may still return null for anything it cannot load.
std::unique_ptr<fx_font> fx_backend_create_font(std::string_view family,
                                                float size,
                                                uint32_t attrs);
std::unique_ptr<fx_font> fx_backend_create_font_from_file(std::string_view path,
                                                          float size,
                                                          uint32_t attrs);

// These are worth reaching for directly rather than through fx_font. Driving them from the public
// API means every iteration loads a font and runs a real shaper and rasterizer: measured at ~925
// execs/s against ~5400 here, and most of that time is spent inside Core Text, DirectWrite or
// Pango, where a crash would be a bug we cannot fix. The functions below are ours, they are pure,
// and their contracts are checkable.

// Bounds of everything the rasterizer changed, as an exclusive rect. Empty when every pixel still
// equals `background`. The result is tight: each edge touches at least one changed pixel.
recti find_ink(const fx_pixel_buffer& buffer, uint32_t background);

// Turns what the rasterizer drew into the coverage the renderers expect: `ramp` over the three
// color channels, then, when `invert` is set, the XOR that folds an alternate glyph's
// black-on-white back to coverage. A null `ramp` with `invert` clear is a no-op. Never touches the
// alpha byte.
void apply_coverage(std::span<uint32_t> pixels, const fx_gamma_ramp* ramp, bool invert);

// Sublime's separable glow (0x1002abca8), applied in place over the ink grown by half the radius.
// `radius` is in whole device pixels; anything below two leaves the buffer untouched. The caller
// owns the conversion from its fractional radius, because the padding it allocates rounds up while
// the kernel here rounds down.
void fx_apply_font_glow(fx_pixel_buffer* buffer, int radius, bool colored);
