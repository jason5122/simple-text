#pragma once

#include "util/non_copyable.h"
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class FileWatcherCallback {
public:
    FileWatcherCallback() = default;
    virtual ~FileWatcherCallback() = default;

    virtual void onFileEvent() {}
};

class FileWatcher : util::NonMovable {
public:
    FileWatcher(fs::path directory, FileWatcherCallback* callback);
    ~FileWatcher();

    bool start();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};
