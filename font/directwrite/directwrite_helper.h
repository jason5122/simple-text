#pragma once

#include <dwrite_3.h>
#include <string>
#include <wrl/client.h>

namespace font {

inline std::wstring GetFontFamilyName(IDWriteFont* font) {
    Microsoft::WRL::ComPtr<IDWriteFontFamily> font_family;
    font->GetFontFamily(&font_family);

    Microsoft::WRL::ComPtr<IDWriteLocalizedStrings> family_names;
    font_family->GetFamilyNames(&family_names);

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH);

    UINT32 index = 0;
    BOOL exists = false;
    family_names->FindLocaleName(localeName, &index, &exists);

    UINT32 length = 0;
    family_names->GetStringLength(index, &length);

    std::wstring name;
    name.resize(length + 1);
    family_names->GetString(index, name.data(), length + 1);
    return name;
}

}  // namespace font
