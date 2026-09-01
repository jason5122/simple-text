#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "experiments/platform/fx/font_private.h"
#include "experiments/platform/fx/fx.h"
#include <windows.h>
// clang-format off: windows.h supplies the GDI types dwrite_2.h uses in its bitmap render target.
#include <dwrite_2.h>
#include <wrl/client.h>
// clang-format on
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <spdlog/spdlog.h>
#include <utility>

// DirectWrite backend, written to mirror Sublime Text's. Behavioural notes below cite addresses in
// the Windows build disassembled under conformance/ (sublime_text.exe, build 4200 x64).
//
// The shape is: Sublime never touches IDWriteTextAnalyzer or IDWriteFontFallback. It hands a
// string to IDWriteTextLayout, implements IDWriteTextRenderer, and scrapes the glyph runs that
// IDWriteTextLayout::Draw calls back with -- so DirectWrite performs both shaping and font
// fallback, and Sublime only records which face each run resolved to.

using Microsoft::WRL::ComPtr;

namespace fx_detail {

namespace {

// Sublime's font_options bit field (direct_write_font+0x38), decoded from the settings parser at
// 0x14013b438. Bits 0-1 are the requested style rather than a user-facing option; bits 7-8
// ("no_bold"/"no_italic") are applied elsewhere and never reach this field.
enum FontFlags : uint32_t {
    kBold = 1u << 0,
    kItalic = 1u << 1,
    kNoAntialias = 1u << 2,
    kGrayAntialias = 1u << 3,
    kSubpixelAntialias = 1u << 4,
    kDirectWrite = 1u << 5,
    kGdi = 1u << 6,
    kClearTypeClassic = 1u << 9,   // "dwrite_cleartype_classic"
    kClearTypeNatural = 1u << 10,  // "dwrite_cleartype_natural"
    kNoLiga = 1u << 11,
    kNoClig = 1u << 12,
    kNoCalt = 1u << 13,
    kDlig = 1u << 14,
    kSs01 = 1u << 15,  // ss01..ss10 occupy bits 15-24
    kGdiCompatible = kClearTypeClassic | kClearTypeNatural,
};

// Sublime's default: ClearType, which puts a different coverage value in each of R/G/B. The
// renderer consumes all three via dual-source blending, so no flag is needed here. Setting
// kGrayAntialias collapses it to a single coverage, which is what a renderer without dual-source
// blending would need.
constexpr uint32_t kDefaultFlags = 0;

// Sublime rounds with floor(x + 0.4999999999999998) rather than std::round (0x1401bba8f). The
// nudged constant avoids the double-rounding std::round can hit, and it breaks ties downward where
// std::round breaks them away from zero -- which changes the pixel a baseline lands on.
double st_round(double x) {
    constexpr double kHalf = 0.4999999999999998;
    return x < 0 ? std::ceil(x - kHalf) : std::floor(x + kHalf);
}

bool starts_with_ci(std::string_view s, std::string_view prefix) {
    if (s.size() < prefix.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), s.begin(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

// Process-wide DirectWrite state. Sublime keeps the same set of globals (0x140709270 onwards),
// resolved through LoadLibraryW("dwrite.dll") + GetProcAddress("DWriteCreateFactory") so it can
// fall back to a pure-GDI font when DirectWrite is missing. We link the import library instead:
// the demo has no GDI fallback to offer.
struct Globals {
    ComPtr<IDWriteFactory> factory;
    ComPtr<IDWriteFactory2> factory2;  // null before Windows 8.1; only used for color glyphs
    ComPtr<IDWriteGdiInterop> gdi_interop;
    ComPtr<IDWriteRenderingParams> rendering_params;
};

const Globals& globals() {
    static const Globals g = [] {
        Globals result;
        if (SUCCEEDED(DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2),
                reinterpret_cast<IUnknown**>(result.factory2.GetAddressOf())))) {
            result.factory2.As(&result.factory);
        } else {
            DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                reinterpret_cast<IUnknown**>(result.factory.GetAddressOf()));
        }
        if (result.factory) {
            result.factory->GetGdiInterop(&result.gdi_interop);
            const POINT origin{};
            const HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
            result.factory->CreateMonitorRenderingParams(monitor, &result.rendering_params);
            if (!result.rendering_params) {
                result.factory->CreateRenderingParams(&result.rendering_params);
            }
        }
        return result;
    }();
    return g;
}

struct FaceEntry {
    ComPtr<IDWriteFontFace> face;
    // (ascent + lineGap) in DIPs, unrounded. Sublime records this per fallback face the first time
    // a glyph run resolves to it (0x1401bc928) and uses it to keep every face's baseline on the
    // primary font's.
    float ascent = 0;
};

}  // namespace

struct FontHandle::FontData {
    ComPtr<IDWriteTextFormat> format;
    std::vector<FaceEntry> faces;  // index 0 is the primary face
    std::string family;
    float em_size = 0;
    uint32_t flags = kDefaultFlags;

    // Sublime's direct_write_font+0x3c/0x40/0x44. lineGap lives in the ascent, and line_height is
    // a separate rounding of the whole sum rather than ascent + descent.
    float ascent = 0;
    float raster_ascent = 0;
    float descent = 0;
    float line_height = 0;

    std::optional<bool> monospace;
    FontId id = 0;

    // Sublime caches one bitmap render target per font (+0x28/+0x30) and grows it on demand.
    ComPtr<IDWriteBitmapRenderTarget> target;
    ComPtr<IDWriteBitmapRenderTarget1> target1;
    int target_w = 0;
    int target_h = 0;
};

struct FontHandle::Impl {
    std::shared_ptr<FontData> data;
};

namespace {

using FontData = FontHandle::FontData;

// Appends `face` to the font's registry if it isn't already there and returns its index. Sublime
// does this linear scan inside DrawGlyphRun (0x1401bc7a7); the registry is a member of the font,
// not of one shaping call, so indices stay valid for the font's lifetime.
FontFaceId register_face(FontData& data, IDWriteFontFace* face) {
    for (size_t i = 0; i < data.faces.size(); i++) {
        if (data.faces[i].face.Get() == face) return static_cast<FontFaceId>(i);
    }

    DWRITE_FONT_METRICS metrics{};
    if (data.flags & kGdiCompatible) {
        face->GetGdiCompatibleMetrics(data.em_size, 1.0f, nullptr, &metrics);
    } else {
        face->GetMetrics(&metrics);
    }
    const float upem_scale =
        metrics.designUnitsPerEm ? data.em_size / static_cast<float>(metrics.designUnitsPerEm) : 0;

    data.faces.push_back(
        {face, static_cast<float>(metrics.ascent + metrics.lineGap) * upem_scale});
    return static_cast<FontFaceId>(data.faces.size() - 1);
}

// Sublime's run_visitor (RTTI: `run_visitor<direct_write_font::shape(char16_t const*,
// unsigned __int64)::<lambda_0>>`): an IDWriteTextRenderer that exists only to catch the glyph
// runs IDWriteTextLayout::Draw emits. Draw() invokes the callbacks synchronously and Sublime keeps
// the object on the stack with AddRef/Release stubbed to a bare `return 0` (0x1405d4198), so
// holding references to the caller's locals is safe. We do the same, minus the fake refcount.
class RunVisitor final : public IDWriteTextRenderer {
public:
    RunVisitor(FontData& data, const base::UTF16ToUTF8IndicesMap& indices_map, ShapedText& out)
        : data_(data), indices_map_(indices_map), out_(out) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping) ||
            riid == __uuidof(IDWriteTextRenderer)) {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_FAIL;  // Sublime returns E_FAIL here, not E_NOINTERFACE (0x1401bc712).
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }

    HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*, BOOL* disabled) override {
        *disabled = FALSE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*, DWRITE_MATRIX* transform) override {
        *transform = {1, 0, 0, 1, 0, 0};
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*, FLOAT* pixels_per_dip) override {
        *pixels_per_dip = 1.0f;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*,
                                           FLOAT baseline_origin_x,
                                           FLOAT baseline_origin_y,
                                           DWRITE_MEASURING_MODE,
                                           const DWRITE_GLYPH_RUN* run,
                                           const DWRITE_GLYPH_RUN_DESCRIPTION* desc,
                                           IUnknown*) override {
        if (!run->fontFace || run->glyphCount == 0 || !run->glyphIndices) return S_OK;

        const FontFaceId face = register_face(data_, run->fontFace);
        const size_t n = run->glyphCount;

        std::vector<GlyphPlacement> glyphs(n);
        float pen = 0;
        for (size_t i = 0; i < n; i++) {
            // x is absolute within the layout, matching what CTRunGetPositions gives the Core Text
            // backend: DirectWrite calls back once per run with that run's starting pen position.
            float x = baseline_origin_x + pen;
            // direct_write_font stores this callback coordinate verbatim relative to its rounded
            // public ascent. The renderer later cancels the primary run's constant and applies
            // the device-snapped ascent difference for each fallback face.
            float y = data_.ascent - baseline_origin_y;
            if (run->glyphOffsets) {
                x += run->glyphOffsets[i].advanceOffset;
                y -= run->glyphOffsets[i].ascenderOffset;  // ascenderOffset is y-up
            }
            // glyphAdvances is documented as optional; DirectWrite fills it in for a text layout,
            // but a null one must not be dereferenced.
            const float advance = run->glyphAdvances ? run->glyphAdvances[i] : 0.0f;
            glyphs[i] = {
                .glyph_id = pack_glyph(face, run->glyphIndices[i]),
                .x_advance = advance,
                .x_offset = x,
                .y_offset = y,
                .face_ascent = data_.faces[face].ascent,
                .cluster = 0,
            };
            pen += advance;
        }

        if (desc) map_clusters(*desc, glyphs);
        out_.advance += pen;
        out_.glyphs.insert(out_.glyphs.end(), glyphs.begin(), glyphs.end());
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE
    DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) override {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE
    DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) override {
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE
    DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*) override {
        return S_OK;
    }

private:
    // DirectWrite's clusterMap runs text -> glyph; invert it so each glyph carries the byte offset
    // of the first code point in its cluster. Sublime walks the run description the same way
    // (0x1401bca32), skipping trailing surrogates and keeping the first text position that maps to
    // each new glyph.
    void map_clusters(const DWRITE_GLYPH_RUN_DESCRIPTION& desc,
                      std::vector<GlyphPlacement>& glyphs) const {
        if (!desc.clusterMap || !desc.string) return;

        constexpr size_t kUnset = static_cast<size_t>(-1);
        for (auto& g : glyphs) g.cluster = kUnset;

        size_t previous = kUnset;
        for (UINT32 i = 0; i < desc.stringLength; i++) {
            const UINT32 text_position = desc.textPosition + i;
            const wchar_t unit = desc.string[i];
            if ((unit & 0xFC00) == 0xDC00)
                continue;  // trailing surrogate: same cluster as its lead

            const size_t glyph = desc.clusterMap[i];
            if (glyph >= glyphs.size() || glyph == previous) continue;
            if (text_position >= indices_map_.size()) break;
            glyphs[glyph].cluster = indices_map_[text_position];
            previous = glyph;
        }

        // A cluster that decomposes into several glyphs only names its first, so carry the last
        // known offset forward. Core Text's CTRunGetStringIndices fills these in for us; the
        // cluster map doesn't.
        size_t last = 0;
        for (auto& g : glyphs) {
            if (g.cluster == kUnset) g.cluster = last;
            else last = g.cluster;
        }
    }

    FontData& data_;
    const base::UTF16ToUTF8IndicesMap& indices_map_;
    ShapedText& out_;
};

// The tile Sublime allocates for one glyph, in device pixels, plus where inside it the glyph's
// baseline origin sits. Computed from design metrics only -- Sublime never asks DirectWrite for
// the rasterized bounds (no GetAlphaTextureBounds call anywhere in the binary).
struct Tile {
    int width = 0;
    int height = 0;
    double origin_x = 0;  // tile left -> baseline origin
    double origin_y = 0;  // tile top  -> baseline
};

// Sublime's direct_write_font::glyph_extents (0x1401bb4f8), with render_glyph's ascent adjustment
// folded into origin_y so the result is self-contained.
std::optional<Tile> glyph_tile(const FontData& data, GlyphId glyph, double scale) {
    const FaceEntry& entry = data.faces[face_index_of(glyph)];
    IDWriteFontFace* face = entry.face.Get();

    DWRITE_FONT_METRICS metrics{};
    if (data.flags & kGdiCompatible) {
        face->GetGdiCompatibleMetrics(data.em_size, 1.0f, nullptr, &metrics);
    } else {
        face->GetMetrics(&metrics);
    }
    if (metrics.designUnitsPerEm == 0) return std::nullopt;
    const double upem_scale =
        static_cast<double>(data.em_size / static_cast<float>(metrics.designUnitsPerEm));

    const UINT16 index = glyph_index_of(glyph);
    DWRITE_GLYPH_METRICS gm{};
    if (FAILED(face->GetDesignGlyphMetrics(&index, 1, &gm, FALSE))) return std::nullopt;

    const INT32 ink_w =
        static_cast<INT32>(gm.advanceWidth) - (gm.leftSideBearing + gm.rightSideBearing);
    const INT32 ink_h =
        static_cast<INT32>(gm.advanceHeight) - (gm.topSideBearing + gm.bottomSideBearing);
    if (ink_w <= 0 || ink_h <= 0) return std::nullopt;  // blank glyph, e.g. a space

    // Sublime doubles the ink height before scaling and pads both axes by 2px. The doubling is
    // slack for a rasterizer that can paint outside the design box (hinting, ClearType filtering)
    // without having to measure first; the extra rows come out transparent.
    constexpr double kPad = 2.0;
    constexpr double kBorder = 1.0;
    const double width =
        std::ceil(static_cast<double>(static_cast<float>(ink_w)) * upem_scale * scale) + kPad;
    const double height =
        std::ceil(static_cast<double>(static_cast<float>(ink_h) * 2.0f) * upem_scale * scale) +
        kPad;

    // Vertically the origin is expressed relative to the face's own ascent and then put back by
    // render_glyph, which snaps that ascent to a whole device pixel first (0x1401bba7b). The two
    // terms nearly cancel, leaving the baseline one pixel below the ink top -- but the rounding in
    // between is what decides which pixel row the glyph lands on.
    const double ink_top =
        static_cast<double>(gm.verticalOriginY - gm.topSideBearing) * upem_scale;
    const double ascent = static_cast<double>(entry.ascent);
    const double origin_x =
        std::ceil(static_cast<double>(-gm.leftSideBearing) * upem_scale * scale) + kBorder;
    const double origin_y =
        std::ceil((ink_top - ascent) * scale) + kBorder + st_round(ascent * scale);

    return Tile{
        // One column wider than glyph_extents reports: Sublime's glyph cache adds it when it sizes
        // the buffer (0x1402cd255, `edi = ceil(width) + 1`, used as both width and stride). That
        // column is what holds the trailing antialiased edge once subpixel_x shifts the glyph
        // right -- without it the right stem of d, m and q is cut off.
        .width = base::clamp_ceil<int>(width) + 1,
        .height = base::clamp_ceil<int>(height),
        .origin_x = origin_x,
        .origin_y = origin_y,
    };
}

DWRITE_RENDERING_MODE rendering_mode(uint32_t flags) {
    if (flags & kNoAntialias) return DWRITE_RENDERING_MODE_ALIASED;
    if (flags & kClearTypeClassic) return DWRITE_RENDERING_MODE_GDI_CLASSIC;
    if (flags & kClearTypeNatural) return DWRITE_RENDERING_MODE_GDI_NATURAL;
    return DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC;
}

DWRITE_MEASURING_MODE measuring_mode(uint32_t flags) {
    if (flags & kClearTypeClassic) return DWRITE_MEASURING_MODE_GDI_CLASSIC;
    if (flags & kClearTypeNatural) return DWRITE_MEASURING_MODE_GDI_NATURAL;
    return DWRITE_MEASURING_MODE_NATURAL;
}

// The cache applies the monitor correction after rasterization, as Sublime's gl_glyph_cache does.
// Keep the intermediate coverage mask neutral apart from DirectWrite's normalized contrast term;
// grayscale and aliased modes additionally zero ClearType and flatten the pixel geometry.
ComPtr<IDWriteRenderingParams> params_for(uint32_t flags) {
    const Globals& g = globals();
    if (!g.rendering_params) return g.rendering_params;
    if (!(flags & (kNoAntialias | kGrayAntialias))) {
        // The ordinary DirectWrite path hands the selected monitor parameters straight to
        // IDWriteBitmapRenderTarget::DrawGlyphRun (0x1401bbf2d).  A neutral intermediate mask
        // changes the coverage curve before the glyph-cache gamma table ever sees it.
        return g.rendering_params;
    }

    const bool grayscale = (flags & (kNoAntialias | kGrayAntialias)) != 0;
    const DWRITE_RENDERING_MODE mode = (flags & kNoAntialias)
                                           ? DWRITE_RENDERING_MODE_ALIASED
                                           : g.rendering_params->GetRenderingMode();
    const FLOAT normalized_contrast =
        std::min(g.rendering_params->GetEnhancedContrast(), 4.0f) / 5.0f;
    const FLOAT gamma = 1.0f + normalized_contrast;
    constexpr FLOAT contrast = 0.0f;
    const FLOAT cleartype_level = g.rendering_params->GetClearTypeLevel();

    ComPtr<IDWriteRenderingParams> custom;
    if (FAILED(g.factory->CreateCustomRenderingParams(
            gamma, contrast, grayscale ? 0.0f : cleartype_level,
            grayscale ? DWRITE_PIXEL_GEOMETRY_FLAT : g.rendering_params->GetPixelGeometry(), mode,
            &custom))) {
        return g.rendering_params;
    }
    return custom;
}

// Sublime keeps one render target per font and reallocates at twice the requested size so the next
// few glyphs can reuse it (0x1401bba10).
bool ensure_target(FontData& data, int width, int height, double scale) {
    const Globals& g = globals();
    if (!g.gdi_interop) return false;

    if (!data.target || data.target_w < width || data.target_h < height) {
        data.target.Reset();
        data.target1.Reset();
        data.target_w = width * 2;
        data.target_h = height * 2;
        if (FAILED(g.gdi_interop->CreateBitmapRenderTarget(
                nullptr, static_cast<UINT32>(data.target_w), static_cast<UINT32>(data.target_h),
                &data.target))) {
            data.target_w = data.target_h = 0;
            return false;
        }
        data.target.As(&data.target1);
    }
    // Sublime only sets this when it creates the target, since one font is bound to one scale
    // there. rasterize() takes the scale per call, so keep the target in step with it.
    data.target->SetPixelsPerDip(static_cast<FLOAT>(scale));
    return true;
}

// DirectWrite paints the glyph over black, so each channel already holds that subpixel's coverage
// for a monochrome run. Preserve all three channels for dual-source blending and use their mean
// for destination alpha.
uint32_t coverage_pixel(uint32_t bgra) {
    const uint32_t b = bgra & 0xFF;
    const uint32_t gr = (bgra >> 8) & 0xFF;
    const uint32_t r = (bgra >> 16) & 0xFF;
    return (((b + gr + r) / 3) << 24) | (bgra & 0x00FFFFFF);
}

// Copies the render target's DIB into `out`, which is w*h premultiplied BGRA.
void read_target(IDWriteBitmapRenderTarget* target, int w, int h, std::vector<uint8_t>& out) {
    DIBSECTION dib{};
    HBITMAP bitmap = static_cast<HBITMAP>(GetCurrentObject(target->GetMemoryDC(), OBJ_BITMAP));
    if (GetObjectW(bitmap, sizeof(dib), &dib) == 0 || !dib.dsBm.bmBits) return;

    const auto* src = static_cast<const uint32_t*>(dib.dsBm.bmBits);
    const size_t stride = static_cast<size_t>(dib.dsBm.bmWidthBytes) / 4;
    auto* dst = reinterpret_cast<uint32_t*>(out.data());
    for (size_t y = 0; y < static_cast<size_t>(h); y++) {
        for (size_t x = 0; x < static_cast<size_t>(w); x++) {
            dst[y * static_cast<size_t>(w) + x] = coverage_pixel(src[y * stride + x]);
        }
    }
}

void read_color_target(IDWriteBitmapRenderTarget* target,
                       int w,
                       int h,
                       std::vector<uint8_t>& out) {
    DIBSECTION dib{};
    HBITMAP bitmap = static_cast<HBITMAP>(GetCurrentObject(target->GetMemoryDC(), OBJ_BITMAP));
    if (GetObjectW(bitmap, sizeof(dib), &dib) == 0 || !dib.dsBm.bmBits) return;

    const auto* src = static_cast<const uint32_t*>(dib.dsBm.bmBits);
    const size_t stride = static_cast<size_t>(dib.dsBm.bmWidthBytes) / 4;
    auto* dst = reinterpret_cast<uint32_t*>(out.data());
    for (size_t y = 0; y < static_cast<size_t>(h); y++) {
        for (size_t x = 0; x < static_cast<size_t>(w); x++) {
            const uint32_t pixel = src[y * stride + x];
            const uint32_t b = pixel & 0xFF;
            const uint32_t gr = (pixel >> 8) & 0xFF;
            const uint32_t r = (pixel >> 16) & 0xFF;
            dst[y * static_cast<size_t>(w) + x] = pixel | (((r + gr + b) / 3) << 24);
        }
    }
}

// Fallback for a failed GDI bitmap render target. This exposes DirectWrite's raw three-channel
// coverage, which is the closest available input for the dual-source-blending renderer.
bool rasterize_via_analysis(const FontData& data,
                            const DWRITE_GLYPH_RUN& run,
                            double scale,
                            double origin_x,
                            double origin_y,
                            int w,
                            int h,
                            std::vector<uint8_t>& out) {
    ComPtr<IDWriteGlyphRunAnalysis> analysis;
    if (FAILED(globals().factory->CreateGlyphRunAnalysis(
            &run, static_cast<FLOAT>(scale), nullptr, rendering_mode(data.flags),
            measuring_mode(data.flags), static_cast<FLOAT>(origin_x / scale),
            static_cast<FLOAT>(origin_y / scale), &analysis))) {
        return false;
    }

    const RECT bounds = {0, 0, w, h};
    const bool aliased = (data.flags & kNoAntialias) != 0;
    const DWRITE_TEXTURE_TYPE type =
        aliased ? DWRITE_TEXTURE_ALIASED_1x1 : DWRITE_TEXTURE_CLEARTYPE_3x1;
    const size_t samples = aliased ? 1 : 3;

    std::vector<BYTE> alpha(static_cast<size_t>(w) * static_cast<size_t>(h) * samples);
    if (FAILED(analysis->CreateAlphaTexture(type, &bounds, alpha.data(),
                                            static_cast<UINT32>(alpha.size())))) {
        return false;
    }

    auto* dst = reinterpret_cast<uint32_t*>(out.data());
    for (size_t i = 0; i < static_cast<size_t>(w) * static_cast<size_t>(h); i++) {
        uint32_t rgb = 0;
        for (size_t s = 0; s < 3; s++) {
            const uint32_t v = alpha[i * samples + (samples == 1 ? 0 : s)];
            rgb |= v << (8 * (2 - s));  // CLEARTYPE_3x1 is R, G, B in order
        }
        dst[i] = 0xFF000000 | rgb;
    }
    return true;
}

}  // namespace

FontHandle::FontHandle() = default;
FontHandle::~FontHandle() = default;
FontHandle::FontHandle(FontHandle&& other) = default;
FontHandle& FontHandle::operator=(FontHandle&& other) = default;

FontHandle::FontHandle(std::shared_ptr<FontData> data)
    : impl_(std::make_unique<Impl>(std::move(data))) {}

FontHandle::FontData& FontHandle::data() const { return *impl_->data; }

bool FontHandle::valid() const { return impl_ && impl_->data != nullptr; }
FontId FontHandle::id() const { return impl_->data->id; }
double FontHandle::ascent() const { return impl_->data->ascent; }
double FontHandle::raster_ascent() const { return impl_->data->raster_ascent; }
double FontHandle::descent() const { return impl_->data->descent; }
// Sublime has no leading of its own: lineGap lives in the ascent, and line height is a separate
// rounding of ascent + descent + lineGap rather than the sum of the two rounded values. Reporting
// the difference here makes the public metrics add back up to Sublime's line height.
double FontHandle::leading() const {
    const FontData& data = *impl_->data;
    return data.line_height - data.ascent - data.descent;
}
double FontHandle::size() const { return impl_->data->em_size; }

bool FontHandle::is_monospace() const {
    if (!impl_->data->monospace) {
        impl_->data->monospace =
            std::abs(shape(*this, "i").advance - shape(*this, "M").advance) < 0.001f;
    }
    return *impl_->data->monospace;
}

std::optional<FontHandle> create_font(std::string family,
                                      double size_px,
                                      Weight weight,
                                      Slant slant) {
    return create_font(std::move(family), size_px, weight, slant, kDefaultFlags);
}

std::optional<FontHandle> create_font(
    std::string family, double size_px, Weight weight, Slant slant, uint32_t feature_flags) {
    const Globals& g = globals();
    if (!g.factory || !g.gdi_interop) {
        spdlog::error("DirectWrite is unavailable");
        return std::nullopt;
    }

    auto data = std::make_shared<FontData>();
    data->em_size = static_cast<float>(size_px);
    data->flags = feature_flags;
    if (weight == Weight::Bold) data->flags |= kBold;
    if (slant == Slant::Italic) data->flags |= kItalic;

    // Sublime resolves the family through GDI rather than IDWriteFontCollection::FindFamilyName:
    // it fills a LOGFONTW and calls CreateFontFromLOGFONT (0x1401bd51a, 0x1401bc214), which gets
    // GDI's family aliasing and substitution for free. lfHeight is the negated point size
    // truncated to an integer, and only selects the physical font -- the size DirectWrite renders
    // at comes from CreateTextFormat below.
    // Sublime resolves its own "system" alias to Segoe UI before it ever reaches the font layer
    // (0x1401cebed).
    if (family == "system") family = "Segoe UI";

    LOGFONTW logfont{};
    logfont.lfHeight = -base::clamp_floor<LONG>(size_px);
    logfont.lfWeight = (data->flags & kBold) ? FW_BOLD : 0;
    logfont.lfItalic = (data->flags & kItalic) ? TRUE : FALSE;
    logfont.lfQuality = [&]() -> BYTE {
        if (data->flags & kSubpixelAntialias) return CLEARTYPE_QUALITY;
        if (data->flags & kGrayAntialias) return ANTIALIASED_QUALITY;
        if (data->flags & kNoAntialias) return NONANTIALIASED_QUALITY;
        return DEFAULT_QUALITY;
    }();
    const std::wstring family16 = base::sys_utf8_to_wide(family);
    std::copy_n(family16.begin(), std::min<size_t>(family16.size(), LF_FACESIZE - 1),
                logfont.lfFaceName);

    ComPtr<IDWriteFont> font;
    if (FAILED(g.gdi_interop->CreateFontFromLOGFONT(&logfont, &font))) {
        spdlog::error("could not resolve font family \"{}\"", family);
        return std::nullopt;
    }

    // The family name DirectWrite reports back, not the one that was asked for: Sublime takes
    // index 0 of the localized-name list without consulting the locale (0x1401bc26e).
    ComPtr<IDWriteFontFamily> font_family;
    ComPtr<IDWriteLocalizedStrings> family_names;
    if (FAILED(font->GetFontFamily(&font_family)) ||
        FAILED(font_family->GetFamilyNames(&family_names))) {
        return std::nullopt;
    }
    UINT32 name_length = 0;
    family_names->GetStringLength(0, &name_length);
    std::wstring resolved(name_length + 1, L'\0');
    family_names->GetString(0, resolved.data(), name_length + 1);
    resolved.resize(name_length);
    data->family = base::sys_wide_to_utf8(resolved);

    // CreateFontFromLOGFONT substitutes rather than failing, so an unavailable family comes back
    // as a different one with no error. Sublime reports the same substitution ("font face ...
    // could not be found, defaulting to ...") instead of rendering the wrong face quietly.
    if (!starts_with_ci(data->family, family)) {
        spdlog::warn("font face \"{}\" could not be found, defaulting to \"{}\"", family,
                     data->family);
    }

    if (FAILED(g.factory->CreateTextFormat(resolved.c_str(), nullptr, font->GetWeight(),
                                           font->GetStyle(), font->GetStretch(), data->em_size,
                                           // Sublime hardcodes this locale (0x1406a0690).
                                           L"en-us", &data->format))) {
        return std::nullopt;
    }

    DWRITE_FONT_METRICS metrics{};
    font->GetMetrics(&metrics);
    const float upem_scale = data->em_size / static_cast<float>(metrics.designUnitsPerEm);
    data->raster_ascent = static_cast<float>(metrics.ascent + metrics.lineGap) * upem_scale;
    data->ascent = std::ceil(data->raster_ascent);
    data->descent = std::ceil(static_cast<float>(metrics.descent) * upem_scale);
    data->line_height = std::round(
        static_cast<float>(metrics.ascent + metrics.descent + metrics.lineGap) * upem_scale);
    // A hardcoded Segoe fudge in Sublime (0x1401bc49b), presumably to match some other
    // measurement of the Windows UI font.
    if (starts_with_ci(data->family, "segoe")) {
        data->raster_ascent -= 1.0f;
        data->ascent -= 1.0f;
        data->line_height -= 1.0f;
    }

    // Sublime lets the registry fill itself from the first glyph run. Seeding index 0 here instead
    // keeps a freshly created handle usable before anything has been shaped; DirectWrite resolves
    // the primary run to this same face, so the index is unchanged.
    ComPtr<IDWriteFontFace> face;
    if (FAILED(font->CreateFontFace(&face))) return std::nullopt;
    register_face(*data, face.Get());

    // Process-unique and never reused: a glyph key outliving its font must fail to match, not
    // collide with whatever font is created next.
    static FontId next_id = 1;
    data->id = next_id++;
    return FontHandle(std::move(data));
}

std::optional<FontHandle> create_font(const FontSpec& spec) {
    return create_font(spec.family, spec.size, spec.weight, spec.slant);
}

fx_gamma_ramp rendering_gamma_ramp() {
    fx_gamma_ramp ramp;
    const ComPtr<IDWriteRenderingParams> params = globals().rendering_params;
    // Sublime folds DirectWrite's enhanced-contrast setting into the cache gamma
    // (0x1401bb8dd..0x1401bb91e). Keep the arithmetic in float to match its scalar
    // SSE operations before constructing the byte lookup tables.
    double exponent = 1.0;
    if (params) {
        const float contrast = std::min(params->GetEnhancedContrast(), 4.0f) / 5.0f;
        const float float_exponent = std::max(params->GetGamma() - contrast, 1.0f) - contrast;
        exponent = static_cast<double>(float_exponent);
    }
    for (size_t i = 0; i < ramp.values.size(); ++i) {
        const double input = static_cast<double>(static_cast<float>(i) / 255.0f);
        ramp.values[i] = static_cast<uint8_t>(
            std::min(255.0, std::floor(255.0 * std::pow(input, exponent) + 0.5)));
        ramp.inverse_values[i] = static_cast<uint8_t>(
            std::min(255.0, std::floor(255.0 * std::pow(input, 1.0 / exponent) + 0.5)));
    }
    return ramp;
}

ShapedText shape(const FontHandle& font, std::string_view utf8) {
    FontData& data = font.data();
    const std::wstring utf16 = base::sys_utf8_to_wide(utf8);
    const UINT32 length = base::checked_cast<UINT32>(utf16.size());
    if (length == 0) return {.line_height = data.line_height};

    constexpr FLOAT kUnbounded = std::numeric_limits<FLOAT>::max();
    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr;
    if (data.flags & kGdiCompatible) {
        hr = globals().factory->CreateGdiCompatibleTextLayout(
            utf16.c_str(), length, data.format.Get(), kUnbounded, kUnbounded, 1.0f, nullptr,
            (data.flags & kClearTypeNatural) ? TRUE : FALSE, &layout);
    } else {
        hr = globals().factory->CreateTextLayout(utf16.c_str(), length, data.format.Get(),
                                                 kUnbounded, kUnbounded, &layout);
    }
    if (FAILED(hr)) return {};

    // Sublime always attaches a typography object, and always names liga/clig/calt explicitly --
    // enabled unless the matching font_option turned them off (0x1401bb105). Note that
    // SetTypography replaces DirectWrite's per-script defaults rather than adding to them, so this
    // list is the complete feature set Sublime renders with.
    ComPtr<IDWriteTypography> typography;
    if (SUCCEEDED(globals().factory->CreateTypography(&typography)) && typography) {
        auto feature = [&](DWRITE_FONT_FEATURE_TAG tag, bool on) {
            typography->AddFontFeature({tag, on ? 1u : 0u});
        };
        feature(DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, !(data.flags & kNoLiga));
        feature(DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, !(data.flags & kNoClig));
        feature(DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, !(data.flags & kNoCalt));
        if (data.flags & kDlig) feature(DWRITE_FONT_FEATURE_TAG_DISCRETIONARY_LIGATURES, true);
        // ss01..ss09 differ only in their last character, but ss10 changes two of them, so the
        // tags are spelled out rather than derived by arithmetic.
        static constexpr DWRITE_FONT_FEATURE_TAG kStylisticSets[] = {
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_1, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_2,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_3, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_4,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_5, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_6,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_8,
            DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_9, DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_10,
        };
        for (uint32_t i = 0; i < std::size(kStylisticSets); i++) {
            if (data.flags & (kSs01 << i)) feature(kStylisticSets[i], true);
        }
        layout->SetTypography(typography.Get(), {0, length});
    }

    base::UTF16ToUTF8IndicesMap indices_map;
    if (!indices_map.set_utf8(utf8)) {
        // Malformed UTF-8. Shaping still works (the UTF-16 conversion substitutes replacements),
        // but cluster offsets would be meaningless, so say so rather than report zeroes.
        spdlog::warn("could not map UTF-16 to UTF-8 indices; cluster offsets will be 0");
    }

    const float primary_baseline = std::round(data.faces[0].ascent);
    ShapedText shaped;
    shaped.line_height = data.line_height;
    shaped.primary_y_offset = data.ascent - primary_baseline;
    shaped.primary_face_ascent = data.faces[0].ascent;
    RunVisitor visitor(data, indices_map, shaped);
    layout->Draw(nullptr, &visitor, 0.0f, 0.0f);
    return shaped;
}

GlyphBitmap rasterize(const FontHandle& font, GlyphId glyph, double scale, double subpixel_x) {
    FontData& data = font.data();
    const FontFaceId face_index = face_index_of(glyph);

    if (face_index >= data.faces.size()) return {};

    const std::optional<Tile> tile = glyph_tile(data, glyph, scale);
    if (!tile) return {};

    UINT16 index = glyph_index_of(glyph);
    FLOAT advance = 0;
    DWRITE_GLYPH_OFFSET offset{};
    const DWRITE_GLYPH_RUN run = {
        .fontFace = data.faces[face_index].face.Get(),
        .fontEmSize = data.em_size,
        .glyphCount = 1,
        .glyphIndices = &index,
        .glyphAdvances = &advance,
        .glyphOffsets = &offset,
        .isSideways = FALSE,
        .bidiLevel = 0,
    };

    // subpixel_x arrives in device pixels and only shifts the draw position -- the border absorbs
    // it, so the bearings below stay whole and every phase of a glyph shares one tile size.
    const double origin_x = tile->origin_x + subpixel_x;
    const double origin_y = tile->origin_y;

    ComPtr<IDWriteColorGlyphRunEnumerator> color_layers;
    if (globals().factory2) {
        // Anything other than S_OK (DWRITE_E_NOCOLOR for the common case) means "not a color
        // glyph"; the out pointer is not meaningful then.
        if (FAILED(globals().factory2->TranslateColorGlyphRun(0.0f, 0.0f, &run, nullptr,
                                                              DWRITE_MEASURING_MODE_NATURAL,
                                                              nullptr, 0, &color_layers))) {
            color_layers.Reset();
        }
    }
    bool colored = color_layers != nullptr;

    std::vector<uint8_t> pixels(static_cast<size_t>(tile->width) *
                                static_cast<size_t>(tile->height) * 4);

    // Sublime's primary path is IDWriteBitmapRenderTarget::DrawGlyphRun into a GDI DIB; fall back
    // to CreateGlyphRunAnalysis if the bitmap render target cannot be created (0x1401bb9c1).
    if (ensure_target(data, tile->width, tile->height, scale)) {
        HDC hdc = data.target->GetMemoryDC();
        const RECT rect = {0, 0, tile->width, tile->height};
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        if (data.target1) {
            // Sublime sets this once when it creates the target and again before a color run,
            // which leaves the mode sticky afterwards; setting it per draw keeps mono glyphs
            // rendering the way the font options asked for.
            const bool grayscale = colored || (data.flags & (kNoAntialias | kGrayAntialias));
            data.target1->SetTextAntialiasMode(grayscale ? DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE
                                                         : DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }

        ComPtr<IDWriteRenderingParams> params = params_for(data.flags);
        const auto draw_color_layers = [&](IDWriteColorGlyphRunEnumerator* layers) {
            for (;;) {
                BOOL has_run = FALSE;
                if (FAILED(layers->MoveNext(&has_run)) || !has_run) break;
                const DWRITE_COLOR_GLYPH_RUN* layer = nullptr;
                if (FAILED(layers->GetCurrentRun(&layer))) break;
                if (std::getenv("PX_TRACE_COLOR") && layer->runColor.a != 1.0f) {
                    spdlog::info("color layer glyph {} rgba {:.6f} {:.6f} {:.6f} {:.6f}",
                                 layer->glyphRun.glyphIndices[0], layer->runColor.r,
                                 layer->runColor.g, layer->runColor.b, layer->runColor.a);
                }
                const COLORREF color = RGB(static_cast<int>(layer->runColor.r * 255.0f),
                                           static_cast<int>(layer->runColor.g * 255.0f),
                                           static_cast<int>(layer->runColor.b * 255.0f));
                data.target->DrawGlyphRun(
                    static_cast<FLOAT>(origin_x / scale), static_cast<FLOAT>(origin_y / scale),
                    DWRITE_MEASURING_MODE_NATURAL, &layer->glyphRun, params.Get(), color, nullptr);
            }
        };

        if (colored) {
            draw_color_layers(color_layers.Get());
            // DirectWrite leaves color-layer coverage in the DIB's high byte. Sublime preserves
            // it by ORing in the mean RGB value rather than replacing it
            // (0x1401bbffc..0x1401bc05c).
            read_color_target(data.target.Get(), tile->width, tile->height, pixels);
        } else {
            data.target->DrawGlyphRun(
                static_cast<FLOAT>(origin_x / scale), static_cast<FLOAT>(origin_y / scale),
                measuring_mode(data.flags), &run, params.Get(), RGB(255, 255, 255), nullptr);
            read_target(data.target.Get(), tile->width, tile->height, pixels);
        }
    } else if (rasterize_via_analysis(data, run, scale, origin_x, origin_y, tile->width,
                                      tile->height, pixels)) {
        colored = false;  // this path rasterizes the plain run, color layers and all
    } else {
        return {};
    }

    return {
        .width = static_cast<size_t>(tile->width),
        .height = static_cast<size_t>(tile->height),
        .bearing_x = base::clamp_round<int>(-tile->origin_x),
        .bearing_y = base::clamp_round<int>(-tile->origin_y),
        .colored = colored,
        .pixels = std::move(pixels),
    };
}

}  // namespace fx_detail
