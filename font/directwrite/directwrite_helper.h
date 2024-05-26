#pragma once

#include <d2d1.h>
#include <dwrite_3.h>
#include <wrl/client.h>

#include <format>
#include <iostream>

using Microsoft::WRL::ComPtr;

namespace font {
inline void ColorRunHelper(ID2D1RenderTarget* target,
                           ComPtr<IDWriteColorGlyphRunEnumerator1> color_run_enumerator,
                           UINT origin_y) {
    // TODO: Find a way to reuse render target and brushes.
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> blue_brush = nullptr;
    target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue, 1.0f), &blue_brush);

    D2D1_POINT_2F baseline_origin{
        .x = 0,
        .y = static_cast<FLOAT>(origin_y),
    };

    target->BeginDraw();
    BOOL has_run;
    const DWRITE_COLOR_GLYPH_RUN1* color_run;

    while (true) {
        if (FAILED(color_run_enumerator->MoveNext(&has_run)) || !has_run) {
            break;
        }
        if (FAILED(color_run_enumerator->GetCurrentRun(&color_run))) {
            break;
        }

        switch (color_run->glyphImageFormat) {
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
            if (color_run->paletteIndex == 0xFFFF) {
                layer_brush = blue_brush;
            } else {
                target->CreateSolidColorBrush(color_run->runColor, &layer_brush);
            }

            target->DrawGlyphRun(baseline_origin, &color_run->glyphRun, layer_brush.Get());
            break;
        }
        }
    }
    target->EndDraw();
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
}
