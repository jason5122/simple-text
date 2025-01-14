#pragma once

#include "build/build_config.h"

#include <string>

// Macros for string literal initialization of FilePath::CharType[].
#if BUILDFLAG(IS_WIN)

// The `FILE_PATH_LITERAL_INTERNAL` indirection allows `FILE_PATH_LITERAL` to
// work correctly with macro parameters, for example
// `FILE_PATH_LITERAL(TEST_FILE)` where `TEST_FILE` is a macro #defined as "TestFile".
#define FILE_PATH_LITERAL_INTERNAL(x) L##x
#define FILE_PATH_LITERAL(x) FILE_PATH_LITERAL_INTERNAL(x)

#elif BUILDFLAG(IS_POSIX)
#define FILE_PATH_LITERAL(x) x
#endif

namespace base {

class FilePath {
public:
#if BUILDFLAG(IS_WIN)
    // On Windows, for Unicode-aware applications, native pathnames are wchar_t
    // arrays encoded in UTF-16.
    using StringType = std::wstring;
#elif BUILDFLAG(IS_POSIX)
    // On most platforms, native pathnames are char arrays, and the encoding
    // may or may not be specified.  On Mac OS X, native pathnames are encoded
    // in UTF-8.
    using StringType = std::string;
#endif

    using CharType = StringType::value_type;
    using StringPieceType = std::basic_string_view<CharType>;

    // Null-terminated array of separators used to separate components in paths.
    // Each character in this array is a valid separator, but kSeparators[0] is
    // treated as the canonical separator and is used when composing pathnames.
    static constexpr CharType kSeparators[] =
#if defined(FILE_PATH_USES_WIN_SEPARATORS)
        FILE_PATH_LITERAL("\\/");
#else
        FILE_PATH_LITERAL("/");
#endif

    // std::size(kSeparators), i.e., the number of separators in kSeparators plus
    // one (the null terminator at the end of kSeparators).
    static constexpr size_t kSeparatorsLength = std::size(kSeparators);

    // The special path component meaning "this directory."
    static constexpr CharType kCurrentDirectory[] = FILE_PATH_LITERAL(".");

    // The special path component meaning "the parent directory."
    static constexpr CharType kParentDirectory[] = FILE_PATH_LITERAL("..");

    // The character used to identify a file extension.
    static constexpr CharType kExtensionSeparator = FILE_PATH_LITERAL('.');

    FilePath() = default;
    FilePath(const FilePath& path) = default;
    explicit FilePath(StringPieceType path);

    const StringType& value() const;
    bool empty() const;
    void clear();

private:
    StringType path_;
};

}  // namespace base
