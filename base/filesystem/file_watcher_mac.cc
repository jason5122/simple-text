#include "file_watcher.h"
#include <CoreServices/CoreServices.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

static void FSEventsCallback(ConstFSEventStreamRef stream,
                             void* client_info,
                             size_t num_events,
                             void* event_paths,
                             const FSEventStreamEventFlags event_flags[],
                             const FSEventStreamEventId event_ids[]) {
    FileWatcherCallback* callback = static_cast<FileWatcherCallback*>(client_info);

    char** paths = static_cast<char**>(event_paths);
    for (size_t i = 0; i < num_events; ++i) {
        std::println("Change {} in {}, flags {}", event_ids[i], paths[i], event_flags[i]);

        callback->onFileEvent();
    }
}

class FileWatcher::impl {
public:
    FSEventStreamRef stream;
    dispatch_queue_t queue;
};

FileWatcher::FileWatcher(fs::path directory, FileWatcherCallback* callback) : pimpl{new impl{}} {
    CFStringRef path =
        CFStringCreateWithCString(nullptr, directory.c_str(), kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(nullptr, (const void**)&path, 1, nullptr);

    FSEventStreamContext context{.info = callback};
    pimpl->stream = FSEventStreamCreate(nullptr, &FSEventsCallback, &context, pathsToWatch,
                                        kFSEventStreamEventIdSinceNow, (CFAbsoluteTime)0.1,
                                        kFSEventStreamCreateFlagNone);

    pimpl->queue = dispatch_queue_create("FSEventsQueue", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(pimpl->stream, pimpl->queue);
}

FileWatcher::~FileWatcher() {
    FSEventStreamStop(pimpl->stream);
    FSEventStreamInvalidate(pimpl->stream);
    FSEventStreamRelease(pimpl->stream);
}

bool FileWatcher::start() {
    return FSEventStreamStart(pimpl->stream);
}
