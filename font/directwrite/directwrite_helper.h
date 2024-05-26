#pragma once

#include "font/directwrite/font_fallback_source.h"
#include "font/directwrite/text_analysis.h"
#include <d2d1.h>
#include <dwrite_3.h>
#include <vector>
#include <wrl/client.h>

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
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> black_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 1.0f), &black_brush);
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> blue_brush = nullptr;
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

                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> layer_brush;
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
    Microsoft::WRL::ComPtr<IDWriteFontFamily> font_family;
    font->GetFontFamily(&font_family);

    Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> family_names;
    font_family->GetFamilyNames(&family_names);

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    int defaultLocaleSuccess = GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

    UINT32 index = 0;
    BOOL exists = false;
    family_names->FindLocaleName(localeName, &index, &exists);

    UINT32 length = 0;
    family_names->GetStringLength(index, &length);

    std::wstring name;
    name.resize(length + 1);
    family_names->GetString(index, &name[0], length + 1);
    std::wcerr << std::format(L"family name: {}\n", &name[0]);
}

inline std::wstring ConvertToUTF16(std::string_view utf8_str) {
    // https://stackoverflow.com/a/6693107/14698275
    size_t len = utf8_str.length();
    int required_len = MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, nullptr, 0);

    std::wstring wstr;
    wstr.resize(required_len);
    MultiByteToWideChar(CP_UTF8, 0, &utf8_str[0], len, &wstr[0], required_len);
    return wstr;
}
}
