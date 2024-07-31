#import <Foundation/Foundation.h>

#include "file_reader.h"

fs::path ResourceDir() {
    return NSBundle.mainBundle.resourcePath.fileSystemRepresentation;
}

// https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/AccessingFilesandDirectories/AccessingFilesandDirectories.html
fs::path DataDir() {
    NSFileManager* shared_fm = NSFileManager.defaultManager;
    NSArray* possible_urls = [shared_fm URLsForDirectory:NSApplicationSupportDirectory
                                               inDomains:NSUserDomainMask];
    NSURL* app_support_dir = nil;
    if (possible_urls.count >= 1) {
        app_support_dir = [possible_urls objectAtIndex:0];
    }

    NSURL* app_dir = nil;
    if (app_support_dir) {
        app_dir = [app_support_dir URLByAppendingPathComponent:@"Simple Text"];
    }
    return app_dir.fileSystemRepresentation;
}
