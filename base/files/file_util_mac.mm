#include "base/apple/foundation_util.h"
#include "base/files/file_util.h"
#include <Foundation/Foundation.h>

namespace base {

bool GetTempDir(base::FilePath* path) {
    NSString* tmp = NSTemporaryDirectory();
    if (tmp == nil) {
        return false;
    }
    *path = base::apple::NSStringToFilePath(tmp);
    return true;
}

FilePath GetHomeDir() {
    NSString* tmp = NSHomeDirectory();
    if (tmp != nil) {
        FilePath mac_home_dir = base::apple::NSStringToFilePath(tmp);
        if (!mac_home_dir.empty()) {
            return mac_home_dir;
        }
    }

    // Fall back on temp dir if no home directory is defined.
    FilePath rv;
    if (GetTempDir(&rv)) {
        return rv;
    }

    // Last resort.
    return FilePath("/tmp");
}

}  // namespace base
