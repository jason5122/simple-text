#include "file_util.h"

#include "base/apple/foundation_util.h"

#include <Foundation/Foundation.h>

namespace base {

bool GetTempDir(base::FilePath* path) {
    // In order to facilitate hermetic runs on macOS, first check
    // MAC_CHROMIUM_TMPDIR. This is used instead of TMPDIR for historical reasons.
    // This was originally done for https://crbug.com/698759 (TMPDIR too long for
    // process singleton socket path), but is hopefully obsolete as of
    // https://crbug.com/1266817 (allows a longer process singleton socket path).
    // Continue tracking MAC_CHROMIUM_TMPDIR as that's what build infrastructure
    // sets on macOS.
    const char* env_tmpdir = getenv("MAC_CHROMIUM_TMPDIR");
    if (env_tmpdir) {
        *path = base::FilePath(env_tmpdir);
        return true;
    }

    // If we didn't find it, fall back to the native function.
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
