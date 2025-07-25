#include "base/files/file_util.h"
#include "base/path_service.h"
#include <spdlog/spdlog.h>

namespace base {

namespace {

FilePath ExecutablePath() {
    const char kProcSelfExe[] = "/proc/self/exe";

    FilePath bin_dir;
    if (!ReadSymbolicLink(FilePath(kProcSelfExe), &bin_dir)) {
        spdlog::error("Unable to resolve {}", kProcSelfExe);
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
