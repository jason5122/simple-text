#include "file.h"

#include <sys/stat.h>

static_assert(sizeof(base::stat_wrapper_t::st_size) >= 8);

namespace base {

int File::Stat(const FilePath& path, stat_wrapper_t* sb) {
    return stat(path.value().c_str(), sb);
}
int File::Fstat(int fd, stat_wrapper_t* sb) {
    return fstat(fd, sb);
}
int File::Lstat(const FilePath& path, stat_wrapper_t* sb) {
    return lstat(path.value().c_str(), sb);
}

}  // namespace base
