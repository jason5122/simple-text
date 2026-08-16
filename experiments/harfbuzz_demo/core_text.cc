#include "experiments/harfbuzz_demo/shared.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

namespace {

std::string ToUtf8(CFStringRef string) {
    if (!string) return {};
    CFIndex length = CFStringGetLength(string);
    CFIndex capacity = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::string out(static_cast<size_t>(capacity), '\0');
    if (!CFStringGetCString(string, out.data(), capacity, kCFStringEncodingUTF8)) {
        return {};
    }
    out.resize(std::strlen(out.c_str()));
    return out;
}

// HarfBuzz table-access callback: hand back table bytes from Core Text. tag is a
// big-endian FourCC, matching CTFontTableTag, so it passes through unchanged.
hb_blob_t* ReferenceTable(hb_face_t*, hb_tag_t tag, void* user_data) {
    CTFontRef ct_font = static_cast<CTFontRef>(user_data);
    CFDataRef data =
        CTFontCopyTable(ct_font, static_cast<CTFontTableTag>(tag), kCTFontTableOptionNoOptions);
    if (!data) return nullptr;
    const char* bytes = reinterpret_cast<const char*>(CFDataGetBytePtr(data));
    unsigned length = static_cast<unsigned>(CFDataGetLength(data));
    return hb_blob_create(bytes, length, HB_MEMORY_MODE_READONLY,
                          const_cast<void*>(static_cast<const void*>(data)),
                          [](void* ptr) { CFRelease(static_cast<CFDataRef>(ptr)); });
}

}  // namespace

int main() {
    std::println("HarfBuzz {} + Core Text demo\n", hb_version_string());

    CTFontRef ct_font = CTFontCreateWithName(CFSTR("Helvetica"), kEmPixels, /*matrix=*/nullptr);
    if (!ct_font) {
        std::println(stderr, "Core Text could not create a font.");
        return 1;
    }

    CFStringRef full_name = CTFontCopyFullName(ct_font);
    CFURLRef url = static_cast<CFURLRef>(CTFontCopyAttribute(ct_font, kCTFontURLAttribute));
    CFStringRef path = url ? CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle) : nullptr;
    std::println("Discovered font: {}", ToUtf8(full_name));
    if (path) std::println("Backing file:    {}", ToUtf8(path));
    std::println("");

    hb_face_t* face = hb_face_create_for_tables(
        ReferenceTable, const_cast<void*>(static_cast<const void*>(ct_font)), nullptr);
    hb_font_t* font = MakeHbFont(face);

    std::vector<ShapedGlyph> glyphs = ShapeAndPrint(font, "Rafting");

    // Rasterize with Core Text as white ink on a black (coverage) bitmap.
    CGFloat ascent = CTFontGetAscent(ct_font);
    CGFloat descent = CTFontGetDescent(ct_font);
    double run_width = 0;
    for (const auto& g : glyphs) run_width += g.x_advance;

    const int pad = 3;
    int width = static_cast<int>(std::ceil(run_width)) + 2 * pad;
    int height = static_cast<int>(std::ceil(ascent + descent)) + 2 * pad;

    std::vector<uint8_t> coverage(static_cast<size_t>(width) * height, 0);
    CGColorSpaceRef gray = CGColorSpaceCreateDeviceGray();
    CGContextRef ctx = CGBitmapContextCreate(coverage.data(), width, height,
                                             /*bitsPerComponent=*/8,
                                             /*bytesPerRow=*/width, gray, kCGImageAlphaNone);
    if (ctx) {
        CGContextSetGrayFillColor(ctx, /*gray=*/1.0, /*alpha=*/1.0);  // white ink
        std::vector<CGGlyph> cg_glyphs(glyphs.size());
        std::vector<CGPoint> cg_points(glyphs.size());
        double pen_x = pad;
        for (size_t i = 0; i < glyphs.size(); ++i) {
            cg_glyphs[i] = static_cast<CGGlyph>(glyphs[i].glyph_id);
            cg_points[i] =
                CGPointMake(pen_x + glyphs[i].x_offset, pad + descent + glyphs[i].y_offset);
            pen_x += glyphs[i].x_advance;
        }
        CTFontDrawGlyphs(ct_font, cg_glyphs.data(), cg_points.data(), glyphs.size(), ctx);
        std::println("Rasterized {}x{} bitmap (Core Text):", width, height);
        PrintBitmap(coverage, width, height, width);
        CGContextRelease(ctx);
    } else {
        std::println(stderr, "Failed to create a bitmap context.");
    }

    CGColorSpaceRelease(gray);
    hb_font_destroy(font);
    hb_face_destroy(face);
    if (path) CFRelease(path);
    if (url) CFRelease(url);
    if (full_name) CFRelease(full_name);
    CFRelease(ct_font);
    return 0;
}
