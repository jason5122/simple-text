#pragma once

#include "base/files/file_path.h"
#include "build/build_config.h"

struct stat;

namespace base {

using stat_wrapper_t = struct stat;

class File {
public:
#if BUILDFLAG(IS_POSIX)
    // Wrapper for stat().
    static int Stat(const FilePath& path, stat_wrapper_t* sb);
    // Wrapper for fstat().
    static int Fstat(int fd, stat_wrapper_t* sb);
    // Wrapper for lstat().
    static int Lstat(const FilePath& path, stat_wrapper_t* sb);
#endif
};

}  // namespace base
