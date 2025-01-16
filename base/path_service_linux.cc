#include "path_service.h"

#include <fmt/base.h>

#include "base/files/file_util.h"

namespace base {

namespace {

FilePath ExecutablePath() {
    const char kProcSelfExe[] = "/proc/self/exe";

    FilePath bin_dir;
    if (!ReadSymbolicLink(FilePath(kProcSelfExe), &bin_dir)) {
        fmt::println("Unable to resolve {}", kProcSelfExe);
        std::abort();
    }
    return bin_dir;
}

}  // namespace

FilePath PathService::get_special_path(PathKey key) {
    switch (key) {
    case PathKey::kFileExe:
        return ExecutablePath();
    case PathKey::kDirAssets:
        return ExecutablePath().DirName();
    }
}

}  // namespace base
