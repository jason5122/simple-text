#pragma once

#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class FileWatcherCallback {
public:
    virtual void onFileEvent() {}
};

class FileWatcher {
public:
    FileWatcher(fs::path directory, FileWatcherCallback* callback);
    ~FileWatcher();
    bool start();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};
