#include "base/path_service.h"
#include <spdlog/spdlog.h>
#include <windows.h>

namespace base {

namespace {

FilePath ExecutablePath() {
    // We need to go compute the value. It would be nice to support paths with
    // names longer than MAX_PATH, but the system functions don't seem to be
    // designed for it either, with the exception of GetTempPath (but other
    // things will surely break if the temp path is too long, so we don't bother
    // handling it.
    wchar_t system_buffer[MAX_PATH];
    system_buffer[0] = 0;

    if (::GetModuleFileName(NULL, system_buffer, MAX_PATH) == 0) {
        spdlog::error("GetModuleFileName() error");
        std::abort();
    }
    return FilePath(system_buffer);
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
