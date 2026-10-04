#pragma once

#include "fx/fx.h"
#include <windows.h>
// clang-format off: windows.h supplies the GDI types dwrite_3.h uses in its bitmap render target.
#include <dwrite_3.h>
#include <wrl/client.h>
// clang-format on

// Process-wide DirectWrite state, and the per-monitor rendering parameters derived from it.
// Internal to //fx: fx.h is the library's only public header.

// Sublime keeps the same set of globals (0x140709270 onwards), resolved through
// LoadLibraryW("dwrite.dll") + GetProcAddress("DWriteCreateFactory") so it can fall back to a
// pure-GDI font when DirectWrite is missing. We link the import library instead: the demo has no
// GDI fallback to offer.
struct dwrite_globals {
    Microsoft::WRL::ComPtr<IDWriteFactory> factory;
    Microsoft::WRL::ComPtr<IDWriteFactory2> factory2;  // null before Windows 8.1; color glyphs only
    Microsoft::WRL::ComPtr<IDWriteFactory3> factory3;  // null before Windows 10; font files only
    Microsoft::WRL::ComPtr<IDWriteGdiInterop> gdi_interop;
    // The primary monitor's parameters. Also the fallback for any platform_value naming a display
    // that has never been enumerated, which keeps rasterization outside a window paint working.
    Microsoft::WRL::ComPtr<IDWriteRenderingParams> rendering_params;
};

const dwrite_globals& globals();

// The parameters to draw with for `platform_value`'s monitor under `flags`.
Microsoft::WRL::ComPtr<IDWriteRenderingParams> params_for(uint32_t flags, uint32_t platform_value);

// The glyph cache's post-rasterization gamma correction.
//
// Still derived from the primary monitor, unlike params_for(): fx_font::gamma_ramp() takes no
// platform_value, matching Sublime's argument-less vtable slot. Sublime has the same gap and a
// worse version of it -- its ramp is computed once behind a flag at 0x140708fac, from whichever
// monitor happens to rasterize first.
fx_gamma_ramp rendering_gamma_ramp();
