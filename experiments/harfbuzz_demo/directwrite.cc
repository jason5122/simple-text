#include "experiments/harfbuzz_demo/shared.h"
#include <dwrite.h>
#include <windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {

// Owns a DirectWrite table handle so the hb_blob can release it on destruction.
struct TableContext {
    IDWriteFontFace* face;
    void* context;
};

// HarfBuzz table-access callback: hand back table bytes from DirectWrite.
// HarfBuzz tags are big-endian FourCCs; DirectWrite wants the byte-swapped form.
hb_blob_t* ReferenceTable(hb_face_t*, hb_tag_t tag, void* user_data) {
    IDWriteFontFace* face = static_cast<IDWriteFontFace*>(user_data);
    const void* data = nullptr;
    UINT32 size = 0;
    void* context = nullptr;
    BOOL exists = FALSE;
    HRESULT hr = face->TryGetFontTable(__builtin_bswap32(tag), &data, &size, &context, &exists);
    if (FAILED(hr) || !exists) return nullptr;
    auto* tc = new TableContext{face, context};
    return hb_blob_create(static_cast<const char*>(data), size, HB_MEMORY_MODE_READONLY, tc,
                          [](void* ptr) {
                              auto* t = static_cast<TableContext*>(ptr);
                              t->face->ReleaseFontTable(t->context);
                              delete t;
                          });
}

}  // namespace

int main() {
    std::println("HarfBuzz {} + DirectWrite demo\n", hb_version_string());

    ComPtr<IDWriteFactory> factory;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(factory.GetAddressOf())))) {
        std::println(stderr, "Failed to create a DirectWrite factory.");
        return 1;
    }

    ComPtr<IDWriteFontCollection> collection;
    factory->GetSystemFontCollection(collection.GetAddressOf());

    const wchar_t* family_name = L"Segoe UI";
    UINT32 family_index = 0;
    BOOL exists = FALSE;
    collection->FindFamilyName(family_name, &family_index, &exists);
    if (!exists) {
        std::println(stderr, "Font family not found.");
        return 1;
    }

    ComPtr<IDWriteFontFamily> family;
    collection->GetFontFamily(family_index, family.GetAddressOf());
    ComPtr<IDWriteFont> dwrite_font;
    family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                 DWRITE_FONT_STYLE_NORMAL, dwrite_font.GetAddressOf());
    ComPtr<IDWriteFontFace> face;
    dwrite_font->CreateFontFace(face.GetAddressOf());

    std::println("Discovered font: Segoe UI\n");

    hb_face_t* hb_face = hb_face_create_for_tables(ReferenceTable, face.Get(), nullptr);
    hb_font_t* font = MakeHbFont(hb_face);

    std::vector<ShapedGlyph> glyphs = ShapeAndPrint(font, "Rafting");

    // Rasterize the whole run at once: DirectWrite composites a glyph run into an
    // 8-bit coverage texture, no D2D render target needed.
    std::vector<UINT16> indices(glyphs.size());
    std::vector<FLOAT> advances(glyphs.size());
    std::vector<DWRITE_GLYPH_OFFSET> offsets(glyphs.size());
    for (size_t i = 0; i < glyphs.size(); ++i) {
        indices[i] = static_cast<UINT16>(glyphs[i].glyph_id);
        advances[i] = glyphs[i].x_advance;
        offsets[i] = DWRITE_GLYPH_OFFSET{glyphs[i].x_offset, glyphs[i].y_offset};
    }

    DWRITE_GLYPH_RUN run = {};
    run.fontFace = face.Get();
    run.fontEmSize = static_cast<FLOAT>(kEmPixels);
    run.glyphCount = static_cast<UINT32>(glyphs.size());
    run.glyphIndices = indices.data();
    run.glyphAdvances = advances.data();
    run.glyphOffsets = offsets.data();
    run.isSideways = FALSE;
    run.bidiLevel = 0;

    // Aliased rendering gives a 1-byte-per-pixel (bilevel) coverage mask, the
    // pairing DWRITE_TEXTURE_ALIASED_1x1 requires. Enough to prove rasterization.
    ComPtr<IDWriteGlyphRunAnalysis> analysis;
    if (SUCCEEDED(factory->CreateGlyphRunAnalysis(
            &run, /*pixelsPerDip=*/1.0f, /*transform=*/nullptr, DWRITE_RENDERING_MODE_ALIASED,
            DWRITE_MEASURING_MODE_NATURAL,
            /*baselineOriginX=*/0.0f, /*baselineOriginY=*/0.0f, analysis.GetAddressOf()))) {
        RECT bounds = {};
        analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &bounds);
        int width = bounds.right - bounds.left;
        int height = bounds.bottom - bounds.top;
        if (width > 0 && height > 0) {
            std::vector<uint8_t> coverage(static_cast<size_t>(width) * height);
            analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1, &bounds, coverage.data(),
                                         static_cast<UINT32>(coverage.size()));
            std::println("Rasterized {}x{} bitmap (DirectWrite):", width, height);
            PrintBitmap(coverage, width, height, width);
        }
    } else {
        std::println(stderr, "Failed to create a glyph run analysis.");
    }

    hb_font_destroy(font);
    hb_face_destroy(hb_face);  // releases the table blobs while `face` is alive
    return 0;
}
