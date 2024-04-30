#include "file_reader.h"

#include <limits.h>
#include <unistd.h>

// TODO: See if there is a more native way to describe the resource path.
fs::path ResourceDir() {
    return std::filesystem::canonical("/proc/self/exe").parent_path();
}

fs::path DataDir() {
    if (const char* env_p = std::getenv("XDG_CONFIG_HOME")) {
        fs::path xdg_config_dir = env_p;
        return xdg_config_dir / "simple-text";
    } else {
        fs::path home_dir = std::getenv("HOME");
        return home_dir / ".config" / "simple-text";
    }
}
