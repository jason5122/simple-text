#pragma once

#include "util/not_copyable_or_movable.h"
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class FileWatcherCallback {
public:
    NOT_COPYABLE(FileWatcherCallback)
    NOT_MOVABLE(FileWatcherCallback)
    FileWatcherCallback() = default;
    virtual ~FileWatcherCallback() = default;

    virtual void onFileEvent() {}
};

class FileWatcher {
public:
    NOT_COPYABLE(FileWatcher)
    NOT_MOVABLE(FileWatcher)
    FileWatcher(fs::path directory, FileWatcherCallback* callback);
    ~FileWatcher();

    bool start();

private:
    class impl;
    std::unique_ptr<impl> pimpl;
};
