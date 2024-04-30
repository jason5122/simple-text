#include "file_reader.h"
#include <shlobj_core.h>

// TODO: Consider baking in resources into the .exe.
// https://learn.microsoft.com/en-us/windows/win32/menurc/about-resource-files
fs::path ResourceDir() {
    return fs::path{};
}

fs::path DataDir() {
    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);

    // TODO: This conversion may be problematic. Investigate this.
    fs::path data_dir = path;
    data_dir /= "Simple Text";
    return data_dir;
}
