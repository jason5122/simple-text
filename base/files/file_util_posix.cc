#include "file_util.h"

#include <unistd.h>

#include "base/files/file.h"

namespace base {

FilePath MakeAbsoluteFilePath(const FilePath& input) {
    char full_path[PATH_MAX];
    if (realpath(input.value().c_str(), full_path) == nullptr) {
        return FilePath();
    }
    return FilePath(full_path);
}

bool PathExists(const FilePath& path) {
    return access(path.value().c_str(), F_OK) == 0;
}

bool PathIsReadable(const FilePath& path) {
    return access(path.value().c_str(), R_OK) == 0;
}

bool PathIsWritable(const FilePath& path) {
    return access(path.value().c_str(), W_OK) == 0;
}

bool DirectoryExists(const FilePath& path) {
    stat_wrapper_t file_info;
    if (File::Stat(path, &file_info) != 0) {
        return false;
    }
    return S_ISDIR(file_info.st_mode);
}

bool ReadSymbolicLink(const FilePath& symlink_path, FilePath* target_path) {
    char buf[PATH_MAX];
    ssize_t count = ::readlink(symlink_path.value().c_str(), buf, std::size(buf));

    bool error = count <= 0;
    if (error) {
        target_path->clear();
        return false;
    }

    *target_path = FilePath(FilePath::StringType(buf, static_cast<size_t>(count)));
    return true;
}

}  // namespace base
