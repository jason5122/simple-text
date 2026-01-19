#include "base/location.h"
#include <string>

namespace base {

namespace {

#if defined(__clang__) && defined(_MSC_VER)
static_assert(std::string_view(__FILE__).ends_with("base\\location.cc"));
#else
static_assert(std::string_view(__FILE__).ends_with("base/location.cc"));
#endif

// Keep file names relative to the root folder.
//
// For example, "../../base/file.cc" becoming "base/file.cc" is a common case. The "../../"
// comes from the build folder being something like "out/release".
constexpr size_t stripped_file_path_prefix_length() {
    constexpr char path[] = __FILE__;
    // Only keep the file path starting from the src directory.
#if defined(__clang__) && defined(_MSC_VER)
    constexpr char stripped[] = "base\\location.cc";
#else
    constexpr char stripped[] = "base/location.cc";
#endif
    constexpr size_t path_len = std::char_traits<char>::length(path);
    constexpr size_t stripped_len = std::char_traits<char>::length(stripped);
    static_assert(path_len >= stripped_len, "Invalid file path for base/location.cc.");
    return path_len - stripped_len;
}

constexpr size_t kStrippedPrefixLength = stripped_file_path_prefix_length();

}  // namespace

Location Location::current(const char* function_name, const char* file_name, int line_number) {
    return Location(function_name, file_name + kStrippedPrefixLength, line_number);
}

}  // namespace base
