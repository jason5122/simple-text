#include "font/font_rasterizer.h"

#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/apple/string_conversions.h"
#include "base/numeric/saturation_arithmetic.h"
#include "font/utf16_to_utf8_indices_map.h"
#include <CoreText/CoreText.h>

// TODO: Debug use; remove this.
#include <fmt/base.h>
#include <algorithm>
#include <cassert>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedTypeRef;

namespace font {

namespace {
ScopedCFTypeRef<CTLineRef> CreateCTLine(CTFontRef ct_font, size_t font_id, std::string_view str8);
bool FontSmoothingEnabled();

// Mimic italics by skewing 15 degrees.
// https://mitchellh.com/writing/ghostty-devlog-001#better-make-italics-programmatically
constexpr CGAffineTransform kSyntheticItalicMatrix = {
    .a = 1,
    .b = 0,
    .c = 0.267949,  // Approximately tan(15).
    .d = 1,
    .tx = 0,
    .ty = 0,
};
constexpr bool kUseSyntheticItalic = false;
constexpr bool kUseSyntheticBold = false;
}  // namespace

struct FontRasterizer::NativeFontType {
    ScopedCFTypeRef<CTFontRef> font;
};

class FontRasterizer::impl {};

FontRasterizer::FontRasterizer() {}

FontRasterizer::~FontRasterizer() {}

size_t FontRasterizer::addFont(std::string_view font_name8, int font_size, FontStyle style) {
    std::string font_style;
    if (style == FontStyle::kNone) {
        font_style = "Regular";
    } else if (style == FontStyle::kBold) {
        font_style = "Bold";
    } else if (style == FontStyle::kItalic) {
        font_style = "Italic";
    } else if (style == (FontStyle::kBold | FontStyle::kItalic)) {
        font_style = "Bold Italic";
    }

    auto font_name_cfstring = base::apple::StringToCFString(font_name8);
    auto font_style_cfstring = base::apple::StringToCFString(font_style);
    CFTypeRef keys[] = {
        kCTFontFamilyNameAttribute,
        kCTFontStyleNameAttribute,
    };
    CFTypeRef values[] = {
        font_name_cfstring.get(),
        font_style_cfstring.get(),
    };
    assert(std::size(keys) == std::size(values));
    ScopedCFTypeRef<CFDictionaryRef> attributes =
        CFDictionaryCreate(kCFAllocatorDefault, keys, values, std::size(keys),
                           &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    ScopedCFTypeRef<CTFontDescriptorRef> descriptor =
        CTFontDescriptorCreateWithAttributes(attributes.get());

    const CGAffineTransform* matrix_ptr = kUseSyntheticItalic ? &kSyntheticItalicMatrix : nullptr;

    CTFontRef ct_font = CTFontCreateWithFontDescriptor(descriptor.get(), font_size, matrix_ptr);
    return cacheFont({ct_font}, font_size);
}

size_t FontRasterizer::addSystemFont(int font_size, FontStyle style) {
    bool is_bold = (style & FontStyle::kBold) != FontStyle::kNone;
    CTFontUIFontType font_type = is_bold ? kCTFontUIFontEmphasizedSystem : kCTFontUIFontSystem;

    CTFontRef sys_font = CTFontCreateUIFontForLanguage(font_type, font_size, nullptr);
    if constexpr (kUseSyntheticItalic) {
        const CGAffineTransform* matrix_ptr =
            kUseSyntheticItalic ? &kSyntheticItalicMatrix : nullptr;
        CTFontRef copy = CTFontCreateCopyWithAttributes(sys_font, 0.0, matrix_ptr, nullptr);
        return cacheFont({copy}, font_size);
    } else {
        return cacheFont({sys_font}, font_size);
    }
}

RasterizedGlyph FontRasterizer::rasterize(size_t font_id, uint32_t glyph_id) const {
    CTFontRef font_ref = font_id_to_native[font_id].font.get();

    if (!font_ref) {
        fmt::println("FontRasterizer::rasterize() error: CTFontRef is null!");
        std::abort();
    }

    CGGlyph glyph_index = glyph_id;
    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, &bounds, 1);

    int32_t rasterized_left = std::floor(bounds.origin.x);
    if constexpr (kUseSyntheticBold) {
        // Add width padding for synthetic bold to prevent cutting off.
        // TODO: Check if right side is still being cut off.
        rasterized_left = base::sub_sat(rasterized_left, 2);
    }
    uint32_t rasterized_width = std::ceil(bounds.origin.x - rasterized_left + bounds.size.width);
    int32_t rasterized_descent = std::ceil(-bounds.origin.y);
    int32_t rasterized_ascent = std::ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterized_height = rasterized_descent + rasterized_ascent;
    if constexpr (kUseSyntheticBold) {
        // Add height padding for synthetic bold to prevent cutting off.
        // TODO: Check if bottom is still being cut off.
        rasterized_height += 2;
    }
    int32_t top = std::ceil(bounds.size.height + bounds.origin.y);

    // If the font is a color font and the glyph doesn't have an outline, it is a color glyph.
    // https://github.com/sublimehq/sublime_text/issues/3747#issuecomment-726837744
    bool colored_font = CTFontGetSymbolicTraits(font_ref) & kCTFontTraitColorGlyphs;
    bool has_outline = CTFontCreatePathForGlyph(font_ref, glyph_index, nullptr);
    bool colored = colored_font && !has_outline;

    std::vector<uint8_t> bitmap_data(rasterized_height * rasterized_width * 4);
    ScopedTypeRef<CGColorSpaceRef> color_space_ref{CGColorSpaceCreateDeviceRGB()};
    ScopedTypeRef<CGContextRef> context{CGBitmapContextCreate(
        bitmap_data.data(), rasterized_width, rasterized_height, 8, rasterized_width * 4,
        color_space_ref.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host)};

    CGFloat alpha = colored ? 0.0 : 1.0;
    CGContextSetRGBFillColor(context.get(), 0.0, 0.0, 0.0, alpha);
    CGContextFillRect(context.get(), CGRectMake(0.0, 0.0, rasterized_width, rasterized_height));

    CGContextSetAllowsFontSmoothing(context.get(), true);
    CGContextSetShouldSmoothFonts(context.get(), FontSmoothingEnabled());
    CGContextSetAllowsFontSubpixelQuantization(context.get(), true);
    CGContextSetShouldSubpixelQuantizeFonts(context.get(), true);
    CGContextSetAllowsFontSubpixelPositioning(context.get(), true);
    CGContextSetShouldSubpixelPositionFonts(context.get(), true);
    CGContextSetAllowsAntialiasing(context.get(), true);
    CGContextSetShouldAntialias(context.get(), true);

    CGContextSetRGBFillColor(context.get(), 1.0, 1.0, 1.0, 1.0);
    CGPoint rasterization_origin = CGPointMake(-rasterized_left, rasterized_descent);

    // TODO: Fully implement this.
    if constexpr (kUseSyntheticBold) {
        CGContextSetStrokeColorWithColor(context.get(), CGColorGetConstantColor(kCGColorWhite));
        CGContextSetTextDrawingMode(context.get(), kCGTextFillStroke);
        CGContextSetLineWidth(context.get(), 1.0);
    }

    CTFontDrawGlyphs(font_ref, &glyph_index, &rasterization_origin, 1, context.get());

    return {
        .colored = colored,
        .left = rasterized_left,
        .top = top,
        .width = static_cast<int32_t>(rasterized_width),
        .height = static_cast<int32_t>(rasterized_height),
        .buffer = std::move(bitmap_data),
    };
}

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#195
LineLayout FontRasterizer::layoutLine(size_t font_id, std::string_view str8) {
    assert(std::ranges::count(str8, '\n') == 0);

    UTF16ToUTF8IndicesMap utf8IndicesMap;
    if (!utf8IndicesMap.setUTF8(str8.data(), str8.length())) {
        fmt::println("UTF16ToUTF8IndicesMap::setUTF8 error");
        std::abort();
    }

    CTFontRef ct_font = font_id_to_native[font_id].font.get();
    ScopedCFTypeRef<CTLineRef> ct_line = CreateCTLine(ct_font, font_id, str8);

    int total_advance = 0;
    std::vector<ShapedRun> runs;
    CFArrayRef run_array = CTLineGetGlyphRuns(ct_line.get());
    CFIndex run_count = CFArrayGetCount(run_array);

    for (CFIndex i = 0; i < run_count; ++i) {
        CTRunRef ct_run = static_cast<CTRunRef>(CFArrayGetValueAtIndex(run_array, i));

        auto ct_font = static_cast<CTFontRef>(
            CFDictionaryGetValue(CTRunGetAttributes(ct_run), kCTFontAttributeName));
        auto scoped_ct_font =
            ScopedCFTypeRef<CTFontRef>(ct_font, base::apple::OwnershipPolicy::RETAIN);
        int font_size = CTFontGetSize(ct_font);
        size_t run_font_id = cacheFont({std::move(scoped_ct_font)}, font_size);

        CFIndex glyph_count = CTRunGetGlyphCount(ct_run);
        std::vector<CGGlyph> glyph_ids(glyph_count);
        std::vector<CFIndex> indices(glyph_count);
        std::vector<CGPoint> positions(glyph_count);
        std::vector<CGSize> advances(glyph_count);

        CTRunGetGlyphs(ct_run, {0, glyph_count}, glyph_ids.data());
        CTRunGetStringIndices(ct_run, {0, glyph_count}, indices.data());
        CTRunGetPositions(ct_run, {0, glyph_count}, positions.data());
        CTRunGetAdvances(ct_run, {0, glyph_count}, advances.data());

        std::vector<ShapedGlyph> glyphs;
        glyphs.reserve(glyph_count);
        for (CFIndex i = 0; i < glyph_count; ++i) {
            // TODO: Use subpixel variants instead of rounding.
            Point position = {
                .x = total_advance,
                // .x = static_cast<int>(std::ceil(positions[i].x)),
                .y = static_cast<int>(std::ceil(positions[i].y)),
            };
            Point advance = {
                .x = static_cast<int>(std::ceil(advances[i].width)),
                .y = static_cast<int>(std::ceil(advances[i].height)),
            };

            size_t utf8_index = utf8IndicesMap.mapIndex(indices[i]);
            ShapedGlyph glyph{
                .glyph_id = glyph_ids[i],
                .position = position,
                .advance = advance,
                .index = utf8_index,
            };
            glyphs.emplace_back(std::move(glyph));

            total_advance += advance.x;
        }

        runs.emplace_back(ShapedRun{run_font_id, std::move(glyphs)});
    }

    // TODO: Currently, width != sum of all advances since we round. When we implement subpixel
    // variants, this should no longer be an issue.
    // double width = CTLineGetTypographicBounds(ct_line.get(), nullptr, nullptr, nullptr);

    // TODO: See if this is correct.
    // if (total_advance % 2 == 1) {
    //     ++total_advance;
    // }

    return {
        .layout_font_id = font_id,
        .width = total_advance,
        // .width = static_cast<int>(std::ceil(width)),
        .length = str8.length(),
        .runs = std::move(runs),
    };
}

size_t FontRasterizer::cacheFont(NativeFontType font, int font_size) {
    CTFontRef ct_font = font.font.get();
    ScopedCFTypeRef<CFStringRef> ct_font_name = CTFontCopyPostScriptName(ct_font);
    std::string font_name = base::apple::CFStringToString(ct_font_name.get());

    if (font_name.empty()) {
        fmt::println("FontRasterizer::cacheFont() error: font_name is empty");
        std::abort();
    }

    // If the font is already present, return its ID.
    size_t hash = hashFont(font_name, font_size);
    if (auto it = font_hash_to_id.find(hash); it != font_hash_to_id.end()) {
        return it->second;
    }

    int ascent = std::ceil(CTFontGetAscent(ct_font));
    int descent = std::ceil(CTFontGetDescent(ct_font));
    int leading = std::ceil(CTFontGetLeading(ct_font));

    // Round up to the next even number if odd.
    if (ascent % 2 == 1) ++ascent;
    if (descent % 2 == 1) ++descent;

    int line_height = ascent + descent + leading;

    Metrics metrics{
        .line_height = line_height,
        .ascent = ascent,
        .descent = descent,
        .font_size = font_size,
    };

    size_t font_id = font_hash_to_id.size();
    font_hash_to_id.emplace(hash, font_id);
    font_id_to_native.emplace_back(std::move(font));
    font_id_to_metrics.emplace_back(std::move(metrics));
    return font_id;
}

namespace {
ScopedCFTypeRef<CTLineRef> CreateCTLine(CTFontRef ct_font, size_t font_id, std::string_view str8) {
    ScopedCFTypeRef<CFStringRef> text_string = base::apple::StringToCFStringNoCopy(str8);

    ScopedCFTypeRef<CFMutableDictionaryRef> attr{CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ct_font);

    ScopedCFTypeRef<CFAttributedStringRef> attr_string{
        CFAttributedStringCreate(kCFAllocatorDefault, text_string.get(), attr.get())};

    return CTLineCreateWithAttributedString(attr_string.get());
}

// https://github.com/alacritty/crossfont/blob/9cd8ed05c9cc7ec17fe69183912560f97c050a1a/src/darwin/mod.rs#L275
bool FontSmoothingEnabled() {
    ScopedCFTypeRef<CFPropertyListRef> pref =
        CFPreferencesCopyAppValue(CFSTR("AppleFontSmoothing"), kCFPreferencesCurrentApplication);

    // Case 0: The preference does not exist. By default, macOS smooths fonts.
    if (!pref.get()) {
        return true;
    }
    // Case 1: The preference is an integer. Anything greater than 0 enables smoothing.
    else if (CFGetTypeID(pref.get()) == CFNumberGetTypeID()) {
        CFNumberRef cfnumber = static_cast<CFNumberRef>(pref.get());
        int value;
        CFNumberGetValue(cfnumber, kCFNumberIntType, &value);
        return value != 0;
    }
    // Case 2: The preference is a string. Parse it as an integer.
    else if (CFGetTypeID(pref.get()) == CFStringGetTypeID()) {
        CFStringRef cfstring = static_cast<CFStringRef>(pref.get());
        int value = CFStringGetIntValue(cfstring);
        return value != 0;
    } else {
        return true;
    }
}
}  // namespace

}  // namespace font
