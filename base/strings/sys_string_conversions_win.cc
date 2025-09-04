#include "base/strings/sys_string_conversions.h"
#include <windows.h>

namespace base {

std::string sys_wide_to_utf8(std::wstring_view wide) {
    int wide_length = static_cast<int>(wide.length());
    if (wide_length == 0) return std::string();

    // Compute the length of the buffer we'll need.
    int charcount = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_length, NULL, 0, NULL, NULL);
    if (charcount == 0) return std::string();

    std::string mb;
    mb.resize(static_cast<size_t>(charcount));
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_length, &mb[0], charcount, NULL, NULL);
    return mb;
}

std::wstring sys_utf8_to_wide(std::string_view utf8) {
    if (utf8.empty()) return std::wstring();

    int utf8_length = static_cast<int>(utf8.length());
    // Compute the length of the buffer.
    int charcount = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8_length, NULL, 0);
    if (charcount == 0) return std::wstring();

    std::wstring wide;
    wide.resize(static_cast<size_t>(charcount));
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8_length, &wide[0], charcount);
    return wide;
}

}  // namespace base
