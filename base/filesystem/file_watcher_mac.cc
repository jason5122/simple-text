#include "file_watcher.h"
#include <CoreServices/CoreServices.h>

static void FSEventsCallback(ConstFSEventStreamRef stream, void* client_info, size_t num_events,
                             void* event_paths, const FSEventStreamEventFlags event_flags[],
                             const FSEventStreamEventId event_ids[]) {
    FileWatcher* file_watcher = reinterpret_cast<FileWatcher*>(client_info);

    char** paths = reinterpret_cast<char**>(event_paths);
    for (int i = 0; i < num_events; i++) {
        fprintf(stderr, "Change %llu in %s, flags %u\n", event_ids[i], paths[i], event_flags[i]);

        file_watcher->callback();
    }
}

class FileWatcher::impl {
public:
    FSEventStreamRef stream;
    dispatch_queue_t queue;
};

FileWatcher::FileWatcher(fs::path directory, void (*callback)(void))
    : callback(callback), pimpl{new impl{}} {
    CFStringRef path =
        CFStringCreateWithCString(nullptr, directory.c_str(), kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(nullptr, (const void**)&path, 1, nullptr);

    FSEventStreamContext context{.info = this};
    pimpl->stream = FSEventStreamCreate(nullptr, &FSEventsCallback, &context, pathsToWatch,
                                        kFSEventStreamEventIdSinceNow, (CFAbsoluteTime)0.1,
                                        kFSEventStreamCreateFlagNone);

    pimpl->queue = dispatch_queue_create("FSEventsQueue", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(pimpl->stream, pimpl->queue);
}

FileWatcher::~FileWatcher() {}

bool FileWatcher::start() {
    return FSEventStreamStart(pimpl->stream);
}
