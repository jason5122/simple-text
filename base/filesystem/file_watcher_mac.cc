#include "file_watcher.h"
#include <CoreServices/CoreServices.h>

static void FSEventsCallback(ConstFSEventStreamRef stream, void* client_info, size_t num_events,
                             void* event_paths, const FSEventStreamEventFlags event_flags[],
                             const FSEventStreamEventId event_ids[]) {
    char** paths = reinterpret_cast<char**>(event_paths);
    for (int i = 0; i < num_events; i++) {
        fprintf(stderr, "Change %llu in %s, flags %u\n", event_ids[i], paths[i], event_flags[i]);
    }
}

class FileWatcher::impl {
public:
    FSEventStreamRef stream;
    dispatch_queue_t queue;
};

FileWatcher::FileWatcher(fs::path directory) : pimpl{new impl{}} {
    CFStringRef mypath =
        CFStringCreateWithCString(nullptr, directory.c_str(), kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(nullptr, (const void**)&mypath, 1, nullptr);

    pimpl->stream = FSEventStreamCreate(nullptr, &FSEventsCallback, nullptr, pathsToWatch,
                                        kFSEventStreamEventIdSinceNow, (CFAbsoluteTime)0.1,
                                        kFSEventStreamCreateFlagNone);

    pimpl->queue = dispatch_queue_create("FSEventsQueue", DISPATCH_QUEUE_SERIAL);
    FSEventStreamSetDispatchQueue(pimpl->stream, pimpl->queue);
}

FileWatcher::~FileWatcher() {}

bool FileWatcher::start() {
    return FSEventStreamStart(pimpl->stream);
}
