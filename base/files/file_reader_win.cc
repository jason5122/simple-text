#include "file_reader.h"

#include <shlobj_core.h>

#include "base/windows/unicode.h"

// TODO: Debug use; remove this.
#include <fmt/base.h>
#include <fmt/xchar.h>

namespace base {

// TODO: Implement this.
// TODO: Consider baking in resources into the .exe.
// https://learn.microsoft.com/en-us/windows/win32/menurc/about-resource-files
std::string ResourceDir() {
    std::wstring exe_path(MAX_PATH, L'\0');
    DWORD length = ::GetModuleFileName(nullptr, &exe_path[0], exe_path.size());
    exe_path.resize(length);

    std::string exe_path_utf8 = base::windows::ConvertToUTF8(exe_path);
    fmt::println("exe_path_utf8 = {}", exe_path_utf8);
    return exe_path_utf8;
}

std::string DataDir() {
    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);

    // TODO: Implement this.
    return {};
}

}  // namespace base
