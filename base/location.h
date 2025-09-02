#pragma once

#include "build/build_config.h"
#include <string>

namespace base {

class Location {
public:
    static constexpr Location current(const char* function_name = __builtin_FUNCTION(),
                                      const char* file_name = __builtin_FILE(),
                                      int line_number = __builtin_LINE()) {
        // Keep file names relative to the root folder.
        //
        // For example, "../../base/file.cc" becoming "base/file.cc" is a common case. The "../../"
        // comes from the build folder being something like "out/release".
#if BUILDFLAG(IS_WIN)
        constexpr char kCurrentFile[] = "base\\location.h";
#else
        constexpr char kCurrentFile[] = "base/location.h";
#endif
        constexpr size_t path_len = std::char_traits<char>::length(__FILE__);
        constexpr size_t stripped_len = std::char_traits<char>::length(kCurrentFile);
        static_assert(std::string_view(__FILE__).ends_with(kCurrentFile));
        static_assert(path_len >= stripped_len);
        constexpr size_t kStrippedPrefixLength = path_len - stripped_len;

        return Location(function_name, file_name + kStrippedPrefixLength, line_number);
    }

    const char* function_name() const { return function_name_; }
    const char* file_name() const { return file_name_; }
    int line_number() const { return line_number_; }

private:
    // Constructor should be called with a long-lived char*, such as __FILE__. It assumes the
    // provided value will persist as a global constant, and it will not make a copy of it.
    constexpr Location(const char* function_name, const char* file_name, int line_number)
        : function_name_(function_name), file_name_(file_name), line_number_(line_number) {}

    const char* function_name_ = nullptr;
    const char* file_name_ = nullptr;
    int line_number_ = -1;
};

}  // namespace base
