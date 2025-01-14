#include "file_util.h"

#include <unistd.h>

#include "base/filesystem/file.h"

namespace base {

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

}  // namespace base
