#include "file_util.h"

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

    // TODO: Implement win::ScopedHandle (or just make this compile).
    win::ScopedHandle file(::CreateFile(path_str, desired_access, kFileShareAll, nullptr,
                                        OPEN_EXISTING, flags_and_attrs, nullptr));

    return file.is_valid();
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

}  // namespace base
