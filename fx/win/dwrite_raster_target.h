#pragma once

#include "fx/fx.h"
#include <windows.h>
// clang-format off: windows.h supplies the GDI types dwrite_3.h uses in its bitmap render target.
#include <dwrite_3.h>
#include <wrl/client.h>
// clang-format on

// The GDI surface DirectWrite rasterizes into, and the read-back that turns it into fx coverage.
// Internal to //fx: fx.h is the library's only public header.
//
// Core Text has no counterpart to any of this -- it draws straight into a CGBitmapContext the
// caller already owns. DirectWrite's bitmap path needs a GDI DIB that somebody has to create,
// size, cache and read back, which is what this file is.

// A reusable IDWriteBitmapRenderTarget. Sublime keeps one per font and reallocates at twice the
// requested size so the next few glyphs can reuse it (0x1401bba10).
class dwrite_raster_target {
public:
    // Makes the target at least width x height and bound to `scale`. False when DirectWrite has no
    // GDI interop, or the target could not be created at that size.
    bool ensure(int width, int height, double scale);

    IDWriteBitmapRenderTarget* get() const { return target_.Get(); }
    // Null before Windows 8.1. Only SetTextAntialiasMode needs it.
    IDWriteBitmapRenderTarget1* get1() const { return target1_.Get(); }

    // Copies the DIB into `out`, which is w*h premultiplied BGRA.
    //
    // DirectWrite paints the glyph over black, so for a monochrome run each channel already holds
    // that subpixel's coverage. All three are kept for dual-source blending and their mean becomes
    // the destination alpha. A color run additionally leaves layer coverage in the DIB's high
    // byte, which Sublime keeps by ORing the mean into it rather than replacing it
    // (0x1401bbffc..0x1401bc05c); that is the only difference between the two paths, hence
    // `keep_alpha`.
    void read(int w, int h, bool keep_alpha, fx_pixel_buffer* out) const;

private:
    Microsoft::WRL::ComPtr<IDWriteBitmapRenderTarget> target_;
    Microsoft::WRL::ComPtr<IDWriteBitmapRenderTarget1> target1_;
    int width_ = 0;
    int height_ = 0;
};

// Draws every layer of a color glyph at its own color. DirectWrite hands these back as an
// enumerator rather than a single run, so each layer is a separate DrawGlyphRun at the same origin.
void draw_color_layers(IDWriteBitmapRenderTarget* target,
                       IDWriteColorGlyphRunEnumerator* layers,
                       IDWriteRenderingParams* params,
                       double origin_x,
                       double origin_y);

// Fallback for when no bitmap render target is available, which in practice means a font loaded
// from a file on a system whose GDI interop failed. Exposes DirectWrite's raw three-channel
// coverage, the closest available input for the dual-source-blending renderer.
bool rasterize_via_analysis(const DWRITE_GLYPH_RUN& run,
                            DWRITE_RENDERING_MODE rendering,
                            DWRITE_MEASURING_MODE measuring,
                            double scale,
                            double origin_x,
                            double origin_y,
                            int w,
                            int h,
                            fx_pixel_buffer* out);
