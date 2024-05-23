#pragma once

#include "font/directwrite/font_fallback_source.h"
#include "font/directwrite/text_analysis.h"
#include <d2d1.h>
#include <dwrite_3.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <format>
#include <iostream>

namespace font {
inline void DrawGlyphRunHelper(ID2D1RenderTarget* target, IDWriteFactory4* factory,
                               IDWriteFontFace* fontFace, DWRITE_GLYPH_RUN* glyphRun,
                               UINT origin_y) {
    HRESULT hr = DWRITE_E_NOCOLOR;

    IDWriteColorGlyphRunEnumerator1* colorLayer;

    IDWriteFontFace2* fontFace2;
    fontFace->QueryInterface(reinterpret_cast<IDWriteFontFace2**>(&fontFace2));
    if (fontFace2->IsColorFont()) {
        DWRITE_GLYPH_IMAGE_FORMATS image_formats = DWRITE_GLYPH_IMAGE_FORMATS_COLR;
        hr = factory->TranslateColorGlyphRun({}, glyphRun, nullptr, image_formats,
                                             DWRITE_MEASURING_MODE_NATURAL, nullptr, 0,
                                             &colorLayer);
    }

    // TODO: Find a way to reuse render target and brushes.
    ComPtr<ID2D1SolidColorBrush> black_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &black_brush);
    ComPtr<ID2D1SolidColorBrush> blue_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 1.0f), &blue_brush);

    D2D1_POINT_2F baseline_origin{
        .x = 0,
        .y = static_cast<FLOAT>(origin_y),
    };

    target->BeginDraw();
    if (hr == DWRITE_E_NOCOLOR) {
        target->DrawGlyphRun(baseline_origin, glyphRun, black_brush.Get());
    } else {
        BOOL hasRun;
        const DWRITE_COLOR_GLYPH_RUN1* colorRun;

        while (true) {
            if (FAILED(colorLayer->MoveNext(&hasRun)) || !hasRun) {
                break;
            }
            if (FAILED(colorLayer->GetCurrentRun(&colorRun))) {
                break;
            }

            switch (colorRun->glyphImageFormat) {
            case DWRITE_GLYPH_IMAGE_FORMATS_PNG:
            case DWRITE_GLYPH_IMAGE_FORMATS_JPEG:
            case DWRITE_GLYPH_IMAGE_FORMATS_TIFF:
            case DWRITE_GLYPH_IMAGE_FORMATS_PREMULTIPLIED_B8G8R8A8:
                // std::cerr << "DrawColorBitmapGlyphRun()\n";
                break;

            case DWRITE_GLYPH_IMAGE_FORMATS_SVG:
                // std::cerr << "DrawSvgGlyphRun()\n";
                break;

            case DWRITE_GLYPH_IMAGE_FORMATS_TRUETYPE:
            case DWRITE_GLYPH_IMAGE_FORMATS_CFF:
            case DWRITE_GLYPH_IMAGE_FORMATS_COLR:
            default: {
                // std::cerr << "DrawGlyphRun()\n";

                ComPtr<ID2D1SolidColorBrush> layer_brush;
                if (colorRun->paletteIndex == 0xFFFF) {
                    layer_brush = blue_brush;
                } else {
                    target->CreateSolidColorBrush(colorRun->runColor, &layer_brush);
                }

                target->DrawGlyphRun(baseline_origin, &colorRun->glyphRun, layer_brush.Get());
                break;
            }
            }
        }
    }
    target->EndDraw();
}

inline void PrintFontFamilyName(IDWriteFont* font) {
    IDWriteFontFamily* font_family;
    font->GetFontFamily(&font_family);

    IDWriteLocalizedStrings* family_names;
    font_family->GetFamilyNames(&family_names);

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

    UINT32 index = 0;
    BOOL exists = false;
    family_names->FindLocaleName(localeName, &index, &exists);

    UINT32 length = 0;
    family_names->GetStringLength(index, &length);

    std::wstring name(length + 1, ' ');
    family_names->GetString(index, &name[0], length + 1);
    std::wcerr << std::format(L"family name: {}\n", &name[0]);
}

// https://github.com/linebender/skribo/blob/master/docs/script_matching.md#windows
inline void GetFallbackFont(IDWriteFactory4* factory, std::string_view utf8_str,
                            IDWriteFontFace** selected_font_face, UINT16* glyph_indices) {
    // https://stackoverflow.com/a/6693107/14698275
    size_t len = utf8_str.length();
    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, NULL, 0);
    std::wstring wstr(wchars_num, ' ');
    MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, &wstr[0], wchars_num);

    wchar_t locale[] = L"en-us";

    IDWriteNumberSubstitution* number_substitution;
    factory->CreateNumberSubstitution(DWRITE_NUMBER_SUBSTITUTION_METHOD_NONE, locale, true,
                                      &number_substitution);

    ComPtr<IDWriteTextAnalysisSource> text_analysis =
        new FontFallbackSource(&wstr[0], wchars_num, locale, number_substitution);

    IDWriteFontFallback* fallback;
    factory->GetSystemFontFallback(&fallback);

    UINT32 mapped_len;
    IDWriteFont* mapped_font;
    FLOAT mapped_scale;
    fallback->MapCharacters(text_analysis.Get(), 0, wchars_num, nullptr, nullptr,
                            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                            DWRITE_FONT_STRETCH_NORMAL, &mapped_len, &mapped_font, &mapped_scale);

    if (mapped_font == nullptr) {
        // If no fallback font is found, don't do anything and leave the glyph index as 0.
        // Let the glyph be rendered as the "tofu" glyph.
        std::cerr
            << "IDWriteFontFallback::MapCharacters() warning: No font can render the text.\n";
        return;
    }

    // PrintFontFamilyName(mapped_font);

    IDWriteFontFace* fallback_font_face;
    mapped_font->CreateFontFace(&fallback_font_face);
    *selected_font_face = fallback_font_face;

    IDWriteTextAnalyzer* text_analyzer;
    factory->CreateTextAnalyzer(&text_analyzer);

    TextAnalysis analysis(&wstr[0], wchars_num, nullptr, DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    TextAnalysis::Run* run_head;
    analysis.GenerateResults(text_analyzer, &run_head);

    uint32_t max_glyph_count = 3 * wchars_num / 2 + 16;

    uint16_t* cluster_map = new uint16_t[wchars_num];
    DWRITE_SHAPING_TEXT_PROPERTIES* text_properties;
    text_properties = new DWRITE_SHAPING_TEXT_PROPERTIES[wchars_num];

    uint16_t* out_glyph_indices = new uint16_t[max_glyph_count];
    DWRITE_SHAPING_GLYPH_PROPERTIES* glyph_properties =
        new DWRITE_SHAPING_GLYPH_PROPERTIES[max_glyph_count];
    uint32_t glyph_count;

    // https://github.com/harfbuzz/harfbuzz/blob/2fcace77b2137abb44468a04e87d8716294641a9/src/hb-directwrite.cc#L661
    text_analyzer->GetGlyphs(&wstr[0], wchars_num, *selected_font_face, false, false,
                             &run_head->mScript, locale, nullptr, nullptr, nullptr, 0,
                             max_glyph_count, cluster_map, text_properties, out_glyph_indices,
                             glyph_properties, &glyph_count);

    glyph_indices[0] = out_glyph_indices[0];
}
}
