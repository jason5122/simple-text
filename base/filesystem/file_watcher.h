#pragma once

#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class FileWatcher {
public:
    void (*callback)(void);

    FileWatcher(fs::path directory, void (*callback)(void));
    ~FileWatcher();
    bool start();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};
