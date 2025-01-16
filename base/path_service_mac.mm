#include "path_service.h"

#include <Foundation/Foundation.h>
#include <mach-o/dyld.h>

#include "apple/foundation_util.h"
#include "base/files/file_util.h"

namespace base {

namespace {

FilePath GetExecutablePath() {
    // Executable path can have relative references ("..") depending on how the app was launched.
    uint32_t executable_length = 0;
    _NSGetExecutablePath(nullptr, &executable_length);
    // `executable_length` is the total buffer size required including the NUL
    // terminator, while `basic_string` guarantees that enough space is reserved
    // so that index may be any value between 0 and size() inclusive, though it is
    // UB to set `str[size()]` to anything other than '\0'.
    std::string executable_path(executable_length - 1, '\0');
    int rv = _NSGetExecutablePath(executable_path.data(), &executable_length);

    // _NSGetExecutablePath may return paths containing ./ or ../ which makes FilePath::DirName()
    // work incorrectly, convert it to an absolute path since we expect those to be returned here.
    return MakeAbsoluteFilePath(FilePath(executable_path));
}

FilePath FrameworkBundlePath() {
    return apple::NSStringToFilePath(NSBundle.mainBundle.bundlePath);
}

}  // namespace

FilePath PathService::get_special_path(PathKey key) {
    switch (key) {
    case PathKey::kFileExe:
        return GetExecutablePath();
    case PathKey::kDirAssets:
        return FrameworkBundlePath().Append("Contents").Append("Resources");
    }
}

}  // namespace base
