#include "file_watcher.h"

class FileWatcher::impl {
public:
};

FileWatcher::FileWatcher(fs::path directory, FileWatcherCallback* callback) {}

FileWatcher::~FileWatcher() {}

bool FileWatcher::start() {
    return true;
}
