#include "base/files/file_util.h"
#include "base/strings/sys_string_conversions.h"
#include <stdlib.h>
#include <windows.h>

namespace base {

FilePath MakeAbsoluteFilePath(const FilePath& input) {
    wchar_t file_path[MAX_PATH];
    if (!_wfullpath(file_path, input.value().c_str(), MAX_PATH)) return FilePath();
    return FilePath(file_path);
}

bool PathExists(const FilePath& path) {
    return (::GetFileAttributes(path.value().c_str()) != INVALID_FILE_ATTRIBUTES);
}

namespace {

bool PathHasAccess(const FilePath& path, DWORD dir_desired_access, DWORD file_desired_access) {
    const wchar_t* const path_str = path.value().c_str();
    DWORD fileattr = ::GetFileAttributes(path_str);
    if (fileattr == INVALID_FILE_ATTRIBUTES) return false;

    bool is_directory = fileattr & FILE_ATTRIBUTE_DIRECTORY;
    DWORD desired_access = is_directory ? dir_desired_access : file_desired_access;
    DWORD flags_and_attrs = is_directory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL;

    // TODO: Implement a scoped type for `HANDLE` so we don't have to manually close it.
    constexpr DWORD kFileShareAll = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE file = ::CreateFile(path_str, desired_access, kFileShareAll, nullptr, OPEN_EXISTING,
                               flags_and_attrs, nullptr);
    bool is_valid = file != nullptr;
    ::CloseHandle(file);
    return is_valid;
}

}  // namespace

bool PathIsReadable(const FilePath& path) {
    return PathHasAccess(path, FILE_LIST_DIRECTORY, GENERIC_READ);
}

bool PathIsWritable(const FilePath& path) {
    return PathHasAccess(path, FILE_ADD_FILE, GENERIC_WRITE);
}

bool DirectoryExists(const FilePath& path) {
    DWORD fileattr = ::GetFileAttributes(path.value().c_str());
    if (fileattr != INVALID_FILE_ATTRIBUTES) return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return false;
}

namespace {

// Appends |mode_char| to |mode| before the optional character set encoding; see
// https://msdn.microsoft.com/library/yeby3zcb.aspx for details.
void AppendModeCharacter(wchar_t mode_char, std::wstring* mode) {
    size_t comma_pos = mode->find(L',');
    mode->insert(comma_pos == std::wstring::npos ? mode->length() : comma_pos, 1, mode_char);
}

}  // namespace

FILE* OpenFile(const FilePath& filename, const char* mode) {
    // 'N' is unconditionally added below, so be sure there is not one already
    // present before a comma in |mode|.
    std::wstring w_mode = base::sys_utf8_to_wide(mode);
    AppendModeCharacter(L'N', &w_mode);
    return _wfsopen(filename.value().c_str(), w_mode.c_str(), _SH_DENYNO);
}

}  // namespace base
