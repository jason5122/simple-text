#include "file_util.h"

#include <limits.h>
#include <unistd.h>

std::filesystem::path ResourcePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
}
