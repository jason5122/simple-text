#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "fx/fx.h"
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
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

// DirectWrite backend, written to mirror Sublime Text's. Behavioural notes below cite addresses in
// the Windows build disassembled under conformance/ (sublime_text.exe, build 4200 x64).
//
// The shape is: Sublime never touches IDWriteTextAnalyzer or IDWriteFontFallback. It hands a
// string to IDWriteTextLayout, implements IDWriteTextRenderer, and scrapes the glyph runs that
// IDWriteTextLayout::Draw calls back with -- so DirectWrite performs both shaping and font
// fallback, and Sublime only records which face each run resolved to.

using Microsoft::WRL::ComPtr;

namespace {

namespace {

// DirectWrite-specific bits in Sublime's font_options field, decoded from the settings parser at
// 0x14013b438. Cross-platform style, rasterization, and OpenType feature bits live in fx.h.
enum DirectWriteFlags : uint32_t {
    kDirectWrite = 1u << 5,
    kGdi = 1u << 6,
    kClearTypeClassic = 1u << 9,   // "dwrite_cleartype_classic"
    kClearTypeNatural = 1u << 10,  // "dwrite_cleartype_natural"
    kGdiCompatible = kClearTypeClassic | kClearTypeNatural,
};

// Sublime's default: ClearType, which puts a different coverage value in each of R/G/B. The
// renderer consumes all three via dual-source blending, so no flag is needed here. Setting
// FX_FONT_GRAY_ANTIALIAS collapses it to a single coverage, which is what a renderer without
// dual-source blending would need.
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

}  // namespace

struct Tile {
    int width = 0;
    int height = 0;
    double origin_x = 0;
    double origin_y = 0;
};

fx_gamma_ramp rendering_gamma_ramp();

class direct_write_font final : public fx_font {
public:
    static std::unique_ptr<direct_write_font> create(std::string family,
                                                     float size,
                                                     uint32_t attrs);

    uint32_t attrs() const override { return attrs_; }
    fx_font_metrics metrics() const override;
    float raster_ascent() const override { return raster_ascent_; }
    std::unique_ptr<fx_layout> shape(std::string_view utf8) override;
    std::unique_ptr<fx_layout> shape(std::u32string_view utf32) override;
    void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) override;
    void rasterize(uint32_t glyph,
                   vec2 position,
                   float scale,
                   fx_glyph_bitmap& bitmap,
                   color foreground) override;
    bool is_color_glyph(uint32_t glyph) override;
    bool bg_affects_rasterize() const override { return false; }
    const fx_gamma_ramp* gamma_ramp() const override { return &gamma_; }

private:
    explicit direct_write_font(uint32_t attrs)
        : flags_(attrs), attrs_(attrs), gamma_(rendering_gamma_ramp()) {}

    uint32_t register_face(IDWriteFontFace* face);
    std::optional<Tile> glyph_tile(uint32_t glyph, double scale) const;
    bool ensure_target(int width, int height, double scale);
    bool rasterize_via_analysis(const DWRITE_GLYPH_RUN& run,
                                double scale,
                                double origin_x,
                                double origin_y,
                                int width,
                                int height,
                                std::vector<uint8_t>& out) const;

    ComPtr<IDWriteTextFormat> format;
    // ST stores these as parallel vectors at direct_write_font+0x48 and +0x60. Index 0 is the
    // primary face; shaping appends fallbacks and their unrounded ascents together.
    std::vector<ComPtr<IDWriteFontFace>> faces;
    std::vector<float> face_ascents;
    std::string family;
    float em_size_ = 0;
    uint32_t flags_ = kDefaultFlags;

    // Sublime's direct_write_font+0x3c/0x40/0x44. lineGap lives in the ascent, and line_height is
    // a separate rounding of the whole sum rather than ascent + descent.
    float ascent_ = 0;
    float raster_ascent_ = 0;
    float descent_ = 0;
    float line_height_ = 0;

    // Sublime caches one bitmap render target per font (+0x28/+0x30) and grows it on demand.
    ComPtr<IDWriteBitmapRenderTarget> target;
    ComPtr<IDWriteBitmapRenderTarget1> target1;
    int target_w = 0;
    int target_h = 0;

    uint32_t attrs_ = 0;
    fx_gamma_ramp gamma_;
};

// Appends `face` to the font's registry if it isn't already there and returns its index. Sublime
// does this linear scan inside DrawGlyphRun (0x1401bc7a7); the registry is a member of the font,
// not of one shaping call, so indices stay valid for the font's lifetime.
uint32_t direct_write_font::register_face(IDWriteFontFace* face) {
    for (size_t i = 0; i < faces.size(); i++) {
        if (faces[i].Get() == face) return base::checked_cast<uint32_t>(i);
    }

    DWRITE_FONT_METRICS metrics{};
    if (flags_ & kGdiCompatible) {
        face->GetGdiCompatibleMetrics(em_size_, 1.0f, nullptr, &metrics);
    } else {
        face->GetMetrics(&metrics);
    }
    const float upem_scale =
        metrics.designUnitsPerEm ? em_size_ / static_cast<float>(metrics.designUnitsPerEm) : 0;

    faces.emplace_back(face);
    face_ascents.push_back(static_cast<float>(metrics.ascent + metrics.lineGap) * upem_scale);
    return base::checked_cast<uint32_t>(faces.size() - 1);
}

// ST's RTTI names this run_visitor<direct_write_font::shape(...)::<lambda_0>>. The visitor owns
// only the COM plumbing and forwards each glyph run to the shaping lambda stored by value.
template <typename Callback>
class run_visitor final : public IDWriteTextRenderer {
public:
    explicit run_visitor(Callback callback) : callback_(std::move(callback)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping) ||
            riid == __uuidof(IDWriteTextRenderer)) {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_FAIL;  // Sublime returns E_FAIL here, not E_NOINTERFACE (0x1401bc712).
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 0; }
    ULONG STDMETHODCALLTYPE Release() override { return 0; }

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
        callback_(baseline_origin_x, baseline_origin_y, run, desc);
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
    Callback callback_;
};

// Sublime's direct_write_font::glyph_extents (0x1401bb4f8).
std::optional<Tile> direct_write_font::glyph_tile(uint32_t glyph, double scale) const {
    const uint32_t face_index = glyph >> 16;
    IDWriteFontFace* face = faces[face_index].Get();

    DWRITE_FONT_METRICS metrics{};
    if (flags_ & kGdiCompatible) {
        face->GetGdiCompatibleMetrics(em_size_, 1.0f, nullptr, &metrics);
    } else {
        face->GetMetrics(&metrics);
    }
    if (metrics.designUnitsPerEm == 0) return std::nullopt;
    const double upem_scale =
        static_cast<double>(em_size_ / static_cast<float>(metrics.designUnitsPerEm));

    const UINT16 index = static_cast<uint16_t>(glyph);
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

    // render_glyph adds the face ascent after snapping it to a device pixel (0x1401bba7b).
    // Keep glyph_extents' origin separate because the glyph cache measures its bearing from this
    // value after cropping the rendered bitmap.
    const double ink_top =
        static_cast<double>(gm.verticalOriginY - gm.topSideBearing) * upem_scale;
    const double ascent = static_cast<double>(face_ascents[face_index]);
    const double origin_x =
        std::ceil(static_cast<double>(-gm.leftSideBearing) * upem_scale * scale) + kBorder;
    const double origin_y = std::ceil((ink_top - ascent) * scale) + kBorder;

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
    if (flags & FX_FONT_NO_ANTIALIAS) return DWRITE_RENDERING_MODE_ALIASED;
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
    if (!(flags & (FX_FONT_NO_ANTIALIAS | FX_FONT_GRAY_ANTIALIAS))) {
        // The ordinary DirectWrite path hands the selected monitor parameters straight to
        // IDWriteBitmapRenderTarget::DrawGlyphRun (0x1401bbf2d).  A neutral intermediate mask
        // changes the coverage curve before the glyph-cache gamma table ever sees it.
        return g.rendering_params;
    }

    const bool grayscale = (flags & (FX_FONT_NO_ANTIALIAS | FX_FONT_GRAY_ANTIALIAS)) != 0;
    const DWRITE_RENDERING_MODE mode = (flags & FX_FONT_NO_ANTIALIAS)
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
bool direct_write_font::ensure_target(int width, int height, double scale) {
    const Globals& g = globals();
    if (!g.gdi_interop) return false;

    if (!target || target_w < width || target_h < height) {
        target.Reset();
        target1.Reset();
        target_w = width * 2;
        target_h = height * 2;
        if (FAILED(g.gdi_interop->CreateBitmapRenderTarget(
                nullptr, static_cast<UINT32>(target_w), static_cast<UINT32>(target_h), &target))) {
            target_w = target_h = 0;
            return false;
        }
        target.As(&target1);
    }
    // Sublime only sets this when it creates the target, since one font is bound to one scale
    // there. rasterize() takes the scale per call, so keep the target in step with it.
    target->SetPixelsPerDip(static_cast<FLOAT>(scale));
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
bool direct_write_font::rasterize_via_analysis(const DWRITE_GLYPH_RUN& run,
                                               double scale,
                                               double origin_x,
                                               double origin_y,
                                               int w,
                                               int h,
                                               std::vector<uint8_t>& out) const {
    ComPtr<IDWriteGlyphRunAnalysis> analysis;
    if (FAILED(globals().factory->CreateGlyphRunAnalysis(
            &run, static_cast<FLOAT>(scale), nullptr, rendering_mode(flags_),
            measuring_mode(flags_), static_cast<FLOAT>(origin_x / scale),
            static_cast<FLOAT>(origin_y / scale), &analysis))) {
        return false;
    }

    const RECT bounds = {0, 0, w, h};
    const bool aliased = (flags_ & FX_FONT_NO_ANTIALIAS) != 0;
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

std::unique_ptr<direct_write_font> direct_write_font::create(std::string family,
                                                             float size_px,
                                                             uint32_t attrs) {
    const Globals& g = globals();
    if (!g.factory || !g.gdi_interop) {
        spdlog::error("DirectWrite is unavailable");
        return nullptr;
    }

    auto result = std::unique_ptr<direct_write_font>(new direct_write_font(attrs));
    direct_write_font& data = *result;
    data.em_size_ = size_px;

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
    logfont.lfWeight = (data.flags_ & FX_FONT_BOLD) ? FW_BOLD : 0;
    logfont.lfItalic = (data.flags_ & FX_FONT_ITALIC) ? TRUE : FALSE;
    logfont.lfQuality = [&]() -> BYTE {
        if (data.flags_ & FX_FONT_SUBPIXEL_ANTIALIAS) return CLEARTYPE_QUALITY;
        if (data.flags_ & FX_FONT_GRAY_ANTIALIAS) return ANTIALIASED_QUALITY;
        if (data.flags_ & FX_FONT_NO_ANTIALIAS) return NONANTIALIASED_QUALITY;
        return DEFAULT_QUALITY;
    }();
    const std::wstring family16 = base::sys_utf8_to_wide(family);
    std::copy_n(family16.begin(), std::min<size_t>(family16.size(), LF_FACESIZE - 1),
                logfont.lfFaceName);

    ComPtr<IDWriteFont> font;
    if (FAILED(g.gdi_interop->CreateFontFromLOGFONT(&logfont, &font))) {
        spdlog::error("could not resolve font family \"{}\"", family);
        return nullptr;
    }

    // The family name DirectWrite reports back, not the one that was asked for: Sublime takes
    // index 0 of the localized-name list without consulting the locale (0x1401bc26e).
    ComPtr<IDWriteFontFamily> font_family;
    ComPtr<IDWriteLocalizedStrings> family_names;
    if (FAILED(font->GetFontFamily(&font_family)) ||
        FAILED(font_family->GetFamilyNames(&family_names))) {
        return nullptr;
    }
    UINT32 name_length = 0;
    family_names->GetStringLength(0, &name_length);
    std::wstring resolved(name_length + 1, L'\0');
    family_names->GetString(0, resolved.data(), name_length + 1);
    resolved.resize(name_length);
    data.family = base::sys_wide_to_utf8(resolved);

    // CreateFontFromLOGFONT substitutes rather than failing, so an unavailable family comes back
    // as a different one with no error. Sublime reports the same substitution ("font face ...
    // could not be found, defaulting to ...") instead of rendering the wrong face quietly.
    if (!starts_with_ci(data.family, family)) {
        spdlog::warn("font face \"{}\" could not be found, defaulting to \"{}\"", family,
                     data.family);
    }

    if (FAILED(g.factory->CreateTextFormat(resolved.c_str(), nullptr, font->GetWeight(),
                                           font->GetStyle(), font->GetStretch(), data.em_size_,
                                           // Sublime hardcodes this locale (0x1406a0690).
                                           L"en-us", &data.format))) {
        return nullptr;
    }

    DWRITE_FONT_METRICS metrics{};
    font->GetMetrics(&metrics);
    const float upem_scale = data.em_size_ / static_cast<float>(metrics.designUnitsPerEm);
    data.raster_ascent_ = static_cast<float>(metrics.ascent + metrics.lineGap) * upem_scale;
    data.ascent_ = std::ceil(data.raster_ascent_);
    data.descent_ = std::ceil(static_cast<float>(metrics.descent) * upem_scale);
    data.line_height_ = std::round(
        static_cast<float>(metrics.ascent + metrics.descent + metrics.lineGap) * upem_scale);
    // A hardcoded Segoe fudge in Sublime (0x1401bc49b), presumably to match some other
    // measurement of the Windows UI font.
    if (starts_with_ci(data.family, "segoe")) {
        data.raster_ascent_ -= 1.0f;
        data.ascent_ -= 1.0f;
        data.line_height_ -= 1.0f;
    }

    // Sublime lets the registry fill itself from the first glyph run. Seeding index 0 here instead
    // keeps a freshly created font usable before anything has been shaped; DirectWrite resolves
    // the primary run to this same face, so the index is unchanged.
    ComPtr<IDWriteFontFace> face;
    if (FAILED(font->CreateFontFace(&face))) return nullptr;
    data.register_face(face.Get());
    return result;
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

std::unique_ptr<fx_layout> direct_write_font::shape(std::string_view utf8) {
    auto shaped = std::make_unique<fx_layout>();
    shaped->line_height = line_height_;

    const std::wstring utf16 = base::sys_utf8_to_wide(utf8);
    const UINT32 length = base::checked_cast<UINT32>(utf16.size());
    if (length == 0) return shaped;

    constexpr FLOAT kUnbounded = std::numeric_limits<FLOAT>::max();
    ComPtr<IDWriteTextLayout> layout;
    HRESULT hr;
    if (flags_ & kGdiCompatible) {
        hr = globals().factory->CreateGdiCompatibleTextLayout(
            utf16.c_str(), length, format.Get(), kUnbounded, kUnbounded, 1.0f, nullptr,
            (flags_ & kClearTypeNatural) ? TRUE : FALSE, &layout);
    } else {
        hr = globals().factory->CreateTextLayout(utf16.c_str(), length, format.Get(), kUnbounded,
                                                 kUnbounded, &layout);
    }
    if (FAILED(hr)) return nullptr;

    // Sublime always attaches a typography object, and always names liga/clig/calt explicitly --
    // enabled unless the matching font_option turned them off (0x1401bb105). Note that
    // SetTypography replaces DirectWrite's per-script defaults rather than adding to them, so this
    // list is the complete feature set Sublime renders with.
    ComPtr<IDWriteTypography> typography;
    if (SUCCEEDED(globals().factory->CreateTypography(&typography)) && typography) {
        auto feature = [&](DWRITE_FONT_FEATURE_TAG tag, bool on) {
            typography->AddFontFeature({tag, on ? 1u : 0u});
        };
        feature(DWRITE_FONT_FEATURE_TAG_STANDARD_LIGATURES, !(flags_ & FX_FONT_NO_LIGA));
        feature(DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_LIGATURES, !(flags_ & FX_FONT_NO_CLIG));
        feature(DWRITE_FONT_FEATURE_TAG_CONTEXTUAL_ALTERNATES, !(flags_ & FX_FONT_NO_CALT));
        if (flags_ & FX_FONT_DLIG) {
            feature(DWRITE_FONT_FEATURE_TAG_DISCRETIONARY_LIGATURES, true);
        }
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
            if (flags_ & (FX_FONT_SS01 << i)) feature(kStylisticSets[i], true);
        }
        layout->SetTypography(typography.Get(), {0, length});
    }

    base::UTF16ToUTF8IndicesMap indices_map;
    if (!indices_map.set_utf8(utf8)) {
        // Malformed UTF-8. Shaping still works (the UTF-16 conversion substitutes replacements),
        // but cluster offsets would be meaningless, so say so rather than report zeroes.
        spdlog::warn("could not map UTF-16 to UTF-8 indices; cluster offsets will be 0");
    }

    auto visit_run = [this, &indices_map, &shaped](
                         FLOAT baseline_origin_x, FLOAT baseline_origin_y,
                         const DWRITE_GLYPH_RUN* run, const DWRITE_GLYPH_RUN_DESCRIPTION* desc) {
        if (!run->fontFace || run->glyphCount == 0 || !run->glyphIndices) return;

        const uint32_t face = register_face(run->fontFace);
        const size_t glyph_count = run->glyphCount;
        std::vector<fx_glyph> glyphs(glyph_count);
        float pen = 0.0f;
        for (size_t i = 0; i < glyph_count; i++) {
            // DirectWrite calls back once per run, so add the run's baseline x to each relative
            // glyph position. Convert its line-top-relative baseline to the baseline-relative
            // convention shared by the fx backends.
            float x = baseline_origin_x + pen;
            float y = std::round(face_ascents[0]) - baseline_origin_y;
            if (run->glyphOffsets) {
                x += run->glyphOffsets[i].advanceOffset;
                y -= run->glyphOffsets[i].ascenderOffset;
            }
            const float advance = run->glyphAdvances ? run->glyphAdvances[i] : 0.0f;
            glyphs[i] = {
                .id = (face << 16) | static_cast<uint32_t>(run->glyphIndices[i]),
                .x_offset = x,
                .y_offset = y,
                .cluster = 0,
            };
            pen += advance;
        }

        // DirectWrite's cluster map runs text -> glyph. Invert it, skipping trailing surrogates,
        // then carry each cluster's first byte offset across any additional decomposed glyphs.
        if (desc && desc->clusterMap && desc->string) {
            constexpr uint32_t kUnset = UINT32_MAX;
            for (auto& glyph : glyphs) glyph.cluster = kUnset;

            size_t previous = kUnset;
            for (UINT32 i = 0; i < desc->stringLength; i++) {
                const UINT32 text_position = desc->textPosition + i;
                const wchar_t unit = desc->string[i];
                if ((unit & 0xFC00) == 0xDC00) continue;

                const size_t glyph = desc->clusterMap[i];
                if (glyph >= glyphs.size() || glyph == previous) continue;
                if (text_position >= indices_map.size()) break;
                glyphs[glyph].cluster = base::checked_cast<uint32_t>(indices_map[text_position]);
                previous = glyph;
            }

            uint32_t last = 0;
            for (auto& glyph : glyphs) {
                if (glyph.cluster == kUnset) glyph.cluster = last;
                else last = glyph.cluster;
            }
        }

        shaped->advance += pen;
        shaped->glyphs.insert(shaped->glyphs.end(), glyphs.begin(), glyphs.end());
    };
    run_visitor visitor(std::move(visit_run));
    layout->Draw(nullptr, &visitor, 0.0f, 0.0f);
    return shaped;
}

void direct_write_font::rasterize(
    uint32_t glyph, vec2 position, float scale, fx_glyph_bitmap& bitmap, color foreground) {
    const uint32_t face_index = glyph >> 16;

    if (face_index >= faces.size() || bitmap.empty() ||
        bitmap.width > static_cast<size_t>(std::numeric_limits<int>::max()) ||
        bitmap.height > static_cast<size_t>(std::numeric_limits<int>::max()) || scale <= 0.0f) {
        return;
    }
    const int width = static_cast<int>(bitmap.width);
    const int height = static_cast<int>(bitmap.height);

    UINT16 index = static_cast<uint16_t>(glyph);
    FLOAT advance = 0;
    DWRITE_GLYPH_OFFSET offset{};
    const DWRITE_GLYPH_RUN run = {
        .fontFace = faces[face_index].Get(),
        .fontEmSize = em_size_,
        .glyphCount = 1,
        .glyphIndices = &index,
        .glyphAdvances = &advance,
        .glyphOffsets = &offset,
        .isSideways = FALSE,
        .bidiLevel = 0,
    };

    const double origin_x = position.x;
    const double origin_y = position.y +
                            st_round(static_cast<double>(face_ascents[face_index]) * scale) -
                            std::round(static_cast<double>(face_ascents[0]) * scale);

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
    const bool colored = color_layers != nullptr;

    // Sublime's primary path is IDWriteBitmapRenderTarget::DrawGlyphRun into a GDI DIB; fall back
    // to CreateGlyphRunAnalysis if the bitmap render target cannot be created (0x1401bb9c1).
    if (ensure_target(width, height, scale)) {
        HDC hdc = target->GetMemoryDC();
        const RECT rect = {0, 0, width, height};
        HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        if (target1) {
            // Sublime sets this once when it creates the target and again before a color run,
            // which leaves the mode sticky afterwards; setting it per draw keeps mono glyphs
            // rendering the way the font options asked for.
            const bool grayscale =
                colored || (flags_ & (FX_FONT_NO_ANTIALIAS | FX_FONT_GRAY_ANTIALIAS));
            target1->SetTextAntialiasMode(grayscale ? DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE
                                                    : DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }

        ComPtr<IDWriteRenderingParams> params = params_for(flags_);
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
                target->DrawGlyphRun(
                    static_cast<FLOAT>(origin_x / scale), static_cast<FLOAT>(origin_y / scale),
                    DWRITE_MEASURING_MODE_NATURAL, &layer->glyphRun, params.Get(), color, nullptr);
            }
        };

        if (colored) {
            draw_color_layers(color_layers.Get());
            // DirectWrite leaves color-layer coverage in the DIB's high byte. Sublime preserves
            // it by ORing in the mean RGB value rather than replacing it
            // (0x1401bbffc..0x1401bc05c).
            read_color_target(target.Get(), width, height, bitmap.pixels);
        } else {
            target->DrawGlyphRun(
                static_cast<FLOAT>(origin_x / scale), static_cast<FLOAT>(origin_y / scale),
                measuring_mode(flags_), &run, params.Get(),
                RGB(foreground.red(), foreground.green(), foreground.blue()), nullptr);
            read_target(target.Get(), width, height, bitmap.pixels);
        }
    } else if (!colored) {
        rasterize_via_analysis(run, scale, origin_x, origin_y, width, height, bitmap.pixels);
    }
}

std::string utf32_to_utf8(std::u32string_view input) {
    std::string output;
    output.reserve(input.size());
    for (uint32_t cp : input) {
        if (cp <= 0x7f) {
            output.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0x10ffff) {
            output.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return output;
}

fx_font_metrics direct_write_font::metrics() const {
    return {
        .ascent = ascent_,
        .descent = descent_,
        .leading = line_height_ - ascent_ - descent_,
        .line_height = line_height_,
    };
}

std::unique_ptr<fx_layout> direct_write_font::shape(std::u32string_view utf32) {
    return shape(utf32_to_utf8(utf32));
}

void direct_write_font::extents(uint32_t glyph, float scale, vec2& origin, vec2& size) {
    const uint32_t face_index = glyph >> 16;
    if (face_index >= faces.size()) {
        origin = {};
        size = {};
        return;
    }

    const std::optional<Tile> tile = glyph_tile(glyph, scale);
    if (!tile) {
        origin = {};
        size = {};
        return;
    }

    origin = {tile->origin_x,
              tile->origin_y + std::round(static_cast<double>(face_ascents[0]) * scale)};
    size = {static_cast<double>(tile->width - 1), static_cast<double>(tile->height)};
}

bool direct_write_font::is_color_glyph(uint32_t glyph) {
    const uint32_t face_index = glyph >> 16;
    if (!globals().factory2 || face_index >= faces.size()) {
        return false;
    }

    UINT16 index = static_cast<uint16_t>(glyph);
    FLOAT advance = 0;
    DWRITE_GLYPH_OFFSET offset{};
    const DWRITE_GLYPH_RUN run = {
        .fontFace = faces[face_index].Get(),
        .fontEmSize = em_size_,
        .glyphCount = 1,
        .glyphIndices = &index,
        .glyphAdvances = &advance,
        .glyphOffsets = &offset,
        .isSideways = FALSE,
        .bidiLevel = 0,
    };
    ComPtr<IDWriteColorGlyphRunEnumerator> color_layers;
    const HRESULT result = globals().factory2->TranslateColorGlyphRun(
        0.0f, 0.0f, &run, nullptr, DWRITE_MEASURING_MODE_NATURAL, nullptr, 0, &color_layers);
    return result == S_OK && color_layers != nullptr;
}

}  // namespace

std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs) {
    return direct_write_font::create(std::string(family), size, attrs);
}
