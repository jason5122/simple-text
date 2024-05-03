#pragma once

#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class FileWatcher {
public:
    FileWatcher(fs::path directory);
    ~FileWatcher();
    bool start();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};
