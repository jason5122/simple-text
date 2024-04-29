#include "file_reader.h"

#include <limits.h>
#include <unistd.h>

// TODO: See if there is a more native way to describe the resource path.
fs::path ResourceDir() {
    return std::filesystem::canonical("/proc/self/exe").parent_path();
}

// TODO: Implement this.
fs::path DataDir() {
    return fs::path{};
}
