#include "base/location.h"
#include "build/build_config.h"
#include <string>

namespace base {

namespace {

#if BUILDFLAG(IS_WIN)
constexpr char kCurrentFile[] = "base\\location.cc";
#else
constexpr char kCurrentFile[] = "base/location.cc";
#endif

// Finds the length of the build folder prefix from the file path.
constexpr size_t calculate_stripped_prefix_length() {
    static_assert(std::string_view(__FILE__).ends_with(kCurrentFile));

    constexpr char path[] = __FILE__;
    // Only keep the file path starting from the src directory.
    constexpr size_t path_len = std::char_traits<char>::length(path);
    constexpr size_t stripped_len = std::char_traits<char>::length(kCurrentFile);
    static_assert(path_len >= stripped_len, "Invalid file path for base/location.cc.");
    return path_len - stripped_len;
}

constexpr size_t kStrippedPrefixLength = calculate_stripped_prefix_length();

}  // namespace

Location::Location(const char* function_name, const char* file_name, int line_number)
    : function_name_(function_name), file_name_(file_name), line_number_(line_number) {}

Location Location::current(const char* function_name, const char* file_name, int line_number) {
    return Location(function_name, file_name + kStrippedPrefixLength, line_number);
}

}  // namespace base
