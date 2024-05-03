#pragma once

#include <filesystem>
#include <memory>

#include "config/color_scheme.h"

namespace fs = std::filesystem;

class FileWatcher {
public:
    void (*callback)(void);

    FileWatcher(fs::path directory, config::ColorScheme* color_scheme);
    ~FileWatcher();
    bool start();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};
