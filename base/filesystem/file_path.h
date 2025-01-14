#pragma once

#include <string>

#include "build/build_config.h"

// Windows-style drive letter support and pathname separator characters can be
// enabled and disabled independently, to aid testing.  These #defines are
// here so that the same setting can be used in both the implementation and
// in the unit test.
#if BUILDFLAG(IS_WIN)
#define FILE_PATH_USES_DRIVE_LETTERS
#define FILE_PATH_USES_WIN_SEPARATORS
#endif

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
    FilePath(const FilePath& other) = default;
    explicit FilePath(StringPieceType path);

    bool operator==(const FilePath& other) const;
    bool operator!=(const FilePath& other) const;

    const StringType& value() const;
    [[nodiscard]] bool empty() const;
    void clear();

    // Returns true if |character| is in kSeparators.
    static bool IsSeparator(CharType character);

    // Returns a FilePath corresponding to the directory containing the path
    // named by this object, stripping away the file component.  If this object
    // only contains one component, returns a FilePath identifying
    // kCurrentDirectory.  If this object already refers to the root directory,
    // returns a FilePath identifying the root directory. Please note that this
    // doesn't resolve directory navigation, e.g. the result for "../a" is "..".
    [[nodiscard]] FilePath DirName() const;

    // Returns a FilePath corresponding to the last path component of this
    // object, either a file or a directory.  If this object already refers to
    // the root directory, returns a FilePath identifying the root directory;
    // this is the only situation in which BaseName will return an absolute path.
    [[nodiscard]] FilePath BaseName() const;

    // Returns the final extension of a file path, or an empty string if the file
    // path has no extension.  In most cases, the final extension of a file path
    // refers to the part of the file path from the last dot to the end (including
    // the dot itself).  For example, this method applied to "/pics/jojo.jpg"
    // and "/pics/jojo." returns ".jpg" and ".", respectively.  However, if the
    // base name of the file path is either "." or "..", this method returns an
    // empty string.
    [[nodiscard]] StringType Extension() const;

    // Returns "C:\pics\jojo" for path "C:\pics\jojo.jpg"
    [[nodiscard]] FilePath RemoveExtension() const;

private:
    // Remove trailing separators from this object.  If the path is absolute, it
    // will never be stripped any more than to refer to the absolute root
    // directory, so "////" will become "/", not "".  A leading pair of
    // separators is never stripped, to support alternate roots.  This is used to
    // support UNC paths on Windows.
    void StripTrailingSeparatorsInternal();

    StringType path_;
};

}  // namespace base
