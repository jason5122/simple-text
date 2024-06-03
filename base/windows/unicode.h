#pragma once

#include <string>
#include <windows.h>

namespace base::windows {

inline std::wstring ConvertToUTF16(std::string_view str8) {
    size_t len = str8.length();
    int required_len = MultiByteToWideChar(CP_UTF8, 0, &str8[0], len, nullptr, 0);

    std::wstring str16;
    str16.resize(required_len);
    MultiByteToWideChar(CP_UTF8, 0, &str8[0], len, &str16[0], required_len);
    return str16;
}

inline std::string ConvertToUTF8(std::wstring_view str16) {
    size_t len = str16.length();
    int required_len =
        WideCharToMultiByte(CP_UTF8, 0, &str16[0], len, nullptr, 0, nullptr, nullptr);

    std::string str8;
    str8.resize(required_len);
    WideCharToMultiByte(CP_UTF8, 0, &str16[0], len, &str8[0], required_len, nullptr, nullptr);
    return str8;
}

}
