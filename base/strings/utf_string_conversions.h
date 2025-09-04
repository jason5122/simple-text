#pragma once

#include <string>
#include <string_view>

namespace base {

[[nodiscard]] std::u16string utf8_to_utf16(std::string_view utf8);
[[nodiscard]] std::string utf16_to_utf8(std::u16string_view utf16);

}  // namespace base
