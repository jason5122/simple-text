#pragma once

#include "base/files/file_path.h"

namespace base {

enum class PathKey {
    kFileExe,
    kDirAssets,
};

// The path service is a global table mapping keys to file system paths.
// TODO: Implement locking for use in multiple threads.

class PathService {
public:
    // Populates |path| with a special directory or file. Returns true on success,
    // in which case |path| is guaranteed to have a non-empty value. On failure,
    // |path| will not be changed.
    //
    // A cache is used internally.
    static bool get(PathKey key, FilePath* path);

private:
    // Gets a system-defined special path. This is only called once, then the value is cached.
    static FilePath get_special_path(PathKey key);
};

}  // namespace base
