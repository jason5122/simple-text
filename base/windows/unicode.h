#pragma once

#include <string>
#include <windows.h>

namespace base::windows {

inline std::wstring convert_to_utf16(std::string_view str8) {
    size_t len = str8.length();
    int required_len = MultiByteToWideChar(CP_UTF8, 0, str8.data(), len, nullptr, 0);

    std::wstring str16;
    str16.resize(required_len);
    MultiByteToWideChar(CP_UTF8, 0, str8.data(), len, str16.data(), required_len);
    return str16;
}

inline std::string convert_to_utf8(std::wstring_view str16) {
    size_t len = str16.length();
    int required_len =
        WideCharToMultiByte(CP_UTF8, 0, str16.data(), len, nullptr, 0, nullptr, nullptr);

    std::string str8;
    str8.resize(required_len);
    WideCharToMultiByte(CP_UTF8, 0, str16.data(), len, str8.data(), required_len, nullptr,
                        nullptr);
    return str8;
}

}  // namespace base::windows
