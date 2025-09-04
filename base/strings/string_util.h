#pragma once

#include <string_view>

namespace base {

// Returns true if |str| contains only valid ASCII character values.
// Note 1: IsStringASCII executes in time determined solely by the
// length of the string, not by its contents, so it is robust against
// timing attacks for all strings of equal length.
// Note 2: IsStringASCII assumes the input is likely all ASCII, and
// does not leave early if it is not the case.
bool IsStringASCII(std::string_view str);
bool IsStringASCII(std::u16string_view str);

}  // namespace base
