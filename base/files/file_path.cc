#include "file_path.h"

#include <algorithm>
#include <array>
#include <numeric>

#if BUILDFLAG(IS_MAC)
#include "base/apple/scoped_cftyperef.h"
#endif

namespace base {

using StringType = FilePath::StringType;
using StringPieceType = FilePath::StringPieceType;

namespace {

const FilePath::CharType kStringTerminator = FILE_PATH_LITERAL('\0');

// If this FilePath contains a drive letter specification, returns the
// position of the last character of the drive letter specification,
// otherwise returns npos.  This can only be true on Windows, when a pathname
// begins with a letter followed by a colon.  On other platforms, this always
// returns npos.
StringPieceType::size_type FindDriveLetter(StringPieceType path);
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
bool EqualDriveLetterCaseInsensitive(StringPieceType a, StringPieceType b);
#endif
[[maybe_unused]] bool AreAllSeparators(const StringType& input);

// Find the position of the '.' that separates the extension from the rest
// of the file name. The position is relative to BaseName(), not value().
// Returns npos if it can't find an extension.
StringType::size_type ExtensionSeparatorPosition(const StringType& path);

}  // namespace

FilePath::FilePath(StringPieceType path) : path_(path) {
    auto pos = path_.find(kStringTerminator);
    if (pos != StringType::npos) {
        path_.erase(pos);
    }
}

bool FilePath::operator==(const FilePath& other) const {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
    return EqualDriveLetterCaseInsensitive(this->path_, other.path_);
#else
    return path_ == other.path_;
#endif
}

bool FilePath::operator!=(const FilePath& other) const {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
    return !EqualDriveLetterCaseInsensitive(this->path_, other.path_);
#else
    return path_ != other.path_;
#endif
}

const FilePath::StringType& FilePath::value() const {
    return path_;
}

bool FilePath::empty() const {
    return path_.empty();
}

void FilePath::clear() {
    path_.clear();
}

bool FilePath::IsSeparator(CharType character) {
    for (size_t i = 0; i < kSeparatorsLength - 1; ++i) {
        if (character == kSeparators[i]) {
            return true;
        }
    }
    return false;
}

FilePath FilePath::DirName() const {
    FilePath new_path(path_);
    new_path.StripTrailingSeparatorsInternal();

    // The drive letter, if any, always needs to remain in the output.  If there
    // is no drive letter, as will always be the case on platforms which do not
    // support drive letters, letter will be npos, or -1, so the comparisons and
    // resizes below using letter will still be valid.
    StringType::size_type letter = FindDriveLetter(new_path.path_);

    StringType::size_type last_separator =
        new_path.path_.find_last_of(kSeparators, StringType::npos, kSeparatorsLength - 1);
    if (last_separator == StringType::npos) {
        // path_ is in the current directory.
        new_path.path_.resize(letter + 1);
    } else if (last_separator == letter + 1) {
        // path_ is in the root directory.
        new_path.path_.resize(letter + 2);
    } else if (last_separator == letter + 2 && IsSeparator(new_path.path_[letter + 1])) {
        // path_ is in "//" (possibly with a drive letter); leave the double
        // separator intact indicating alternate root.
        new_path.path_.resize(letter + 3);
    } else if (last_separator != 0) {
        bool trim_to_basename = true;
#if BUILDFLAG(IS_POSIX)
        // On Posix, more than two leading separators are always collapsed to one.
        // See
        // https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_13
        // So, do not strip any of the separators, let
        // StripTrailingSeparatorsInternal() take care of the extra.
        if (AreAllSeparators(new_path.path_.substr(0, last_separator + 1))) {
            new_path.path_.resize(last_separator + 1);
            trim_to_basename = false;
        }
#endif  // BUILDFLAG(IS_POSIX)
        if (trim_to_basename) {
            // path_ is somewhere else, trim the basename.
            new_path.path_.resize(last_separator);
        }
    }

    new_path.StripTrailingSeparatorsInternal();
    if (!new_path.path_.length()) new_path.path_ = kCurrentDirectory;

    return new_path;
}

FilePath FilePath::BaseName() const {
    FilePath new_path(path_);
    new_path.StripTrailingSeparatorsInternal();

    // The drive letter, if any, is always stripped.
    StringType::size_type letter = FindDriveLetter(new_path.path_);
    if (letter != StringType::npos) {
        new_path.path_.erase(0, letter + 1);
    }

    // Keep everything after the final separator, but if the pathname is only
    // one character and it's a separator, leave it alone.
    StringType::size_type last_separator =
        new_path.path_.find_last_of(kSeparators, StringType::npos, kSeparatorsLength - 1);
    if (last_separator != StringType::npos && last_separator < new_path.path_.length() - 1) {
        new_path.path_.erase(0, last_separator + 1);
    }

    return new_path;
}

StringType FilePath::Extension() const {
    FilePath base(BaseName());
    const auto dot = ExtensionSeparatorPosition(base.path_);
    if (dot == StringType::npos) {
        return StringType();
    } else {
        return base.path_.substr(dot, StringType::npos);
    }
}

FilePath FilePath::RemoveExtension() const {
    if (Extension().empty()) return *this;

    const auto dot = ExtensionSeparatorPosition(path_);
    if (dot == StringType::npos) {
        return *this;
    } else {
        return FilePath(path_.substr(0, dot));
    }
}

#if BUILDFLAG(IS_MAC)
StringType FilePath::GetHFSDecomposedForm(StringPieceType string) {
    apple::ScopedCFTypeRef<CFStringRef> cfstring(CFStringCreateWithBytesNoCopy(
        nullptr, reinterpret_cast<const UInt8*>(string.data()),
        static_cast<CFIndex>(string.length()), kCFStringEncodingUTF8, false, kCFAllocatorNull));
    return GetHFSDecomposedForm(cfstring.get());
}

StringType FilePath::GetHFSDecomposedForm(CFStringRef cfstring) {
    if (!cfstring) {
        return StringType();
    }

    StringType result;
    // Query the maximum length needed to store the result. In most cases this
    // will overestimate the required space. The return value also already
    // includes the space needed for a terminating 0.
    CFIndex length = CFStringGetMaximumSizeOfFileSystemRepresentation(cfstring);
    // Reserve enough space for CFStringGetFileSystemRepresentation to write
    // into. Also set the length to the maximum so that we can shrink it later.
    // (Increasing rather than decreasing it would clobber the string contents!)
    result.reserve(static_cast<size_t>(length));
    result.resize(static_cast<size_t>(length) - 1);
    Boolean success = CFStringGetFileSystemRepresentation(cfstring, &result[0], length);
    if (success) {
        // Reduce result.length() to actual string length.
        result.resize(strlen(result.c_str()));
    } else {
        // An error occurred -> clear result.
        result.clear();
    }
    return result;
}
#endif

void FilePath::StripTrailingSeparatorsInternal() {
    // If there is no drive letter, start will be 1, which will prevent stripping
    // the leading separator if there is only one separator.  If there is a drive
    // letter, start will be set appropriately to prevent stripping the first
    // separator following the drive letter, if a separator immediately follows
    // the drive letter.
    StringType::size_type start = FindDriveLetter(path_) + 2;

    StringType::size_type last_stripped = StringType::npos;
    for (StringType::size_type pos = path_.length(); pos > start && IsSeparator(path_[pos - 1]);
         --pos) {
        // If the string only has two separators and they're at the beginning,
        // don't strip them, unless the string began with more than two separators.
        if (pos != start + 1 || last_stripped == start + 2 || !IsSeparator(path_[start - 1])) {
            path_.resize(pos - 1);
            last_stripped = pos;
        }
    }
}

namespace {

StringPieceType::size_type FindDriveLetter(StringPieceType path) {
#if defined(FILE_PATH_USES_DRIVE_LETTERS)
    // This is dependent on an ASCII-based character set, but that's a
    // reasonable assumption.  iswalpha can be too inclusive here.
    if (path.length() >= 2 && path[1] == L':' &&
        ((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z'))) {
        return 1;
    }
#endif
    return StringType::npos;
}

#if defined(FILE_PATH_USES_DRIVE_LETTERS)

namespace {
// Lookup table for fast ASCII case-insensitive comparison.
inline constexpr std::array<unsigned char, 256> kToLower = []() {
    std::array<unsigned char, 256> table;
    std::iota(table.begin(), table.end(), 0);
    std::iota(table.begin() + size_t{'A'}, table.begin() + size_t{'Z'} + 1, 'a');
    return table;
}();
}  // namespace

bool EqualDriveLetterCaseInsensitive(StringPieceType a, StringPieceType b) {
    size_t a_letter_pos = FindDriveLetter(a);
    size_t b_letter_pos = FindDriveLetter(b);

    if (a_letter_pos == StringType::npos || b_letter_pos == StringType::npos) return a == b;

    StringPieceType a_letter(a.substr(0, a_letter_pos + 1));
    StringPieceType b_letter(b.substr(0, b_letter_pos + 1));

    // TODO: Clean this up.
    static constexpr auto lower = [](auto c) constexpr {
        return kToLower[static_cast<unsigned char>(c)];
    };
    if (std::ranges::equal(a_letter.substr(0, b_letter.size()), b_letter, {}, lower, lower)) {
        return false;
    }
    // if (!StartsWith(a_letter, b_letter, CompareCase::INSENSITIVE_ASCII)) return false;

    StringPieceType a_rest(a.substr(a_letter_pos + 1));
    StringPieceType b_rest(b.substr(b_letter_pos + 1));
    return a_rest == b_rest;
}
#endif

bool AreAllSeparators(const StringType& input) {
    for (auto it : input) {
        if (!FilePath::IsSeparator(it)) return false;
    }
    return true;
}

StringType::size_type ExtensionSeparatorPosition(const StringType& path) {
    // Special case "." and ".."
    if (path == FilePath::kCurrentDirectory || path == FilePath::kParentDirectory) {
        return StringType::npos;
    } else {
        return path.rfind(FilePath::kExtensionSeparator);
    }
}

}  // namespace

}  // namespace base
