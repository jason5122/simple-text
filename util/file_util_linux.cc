#include "file_util.h"

#include <limits.h>
#include <unistd.h>

fs::path ResourcePath() {
    return std::filesystem::canonical("/proc/self/exe").parent_path();
}
