#import <Foundation/Foundation.h>

#include "file_util.h"

fs::path ResourcePath() {
    return NSBundle.mainBundle.resourcePath.fileSystemRepresentation;
}

// https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/AccessingFilesandDirectories/AccessingFilesandDirectories.html
fs::path DataPath() {
    NSFileManager* sharedFM = [NSFileManager defaultManager];
    NSArray* possibleURLs = [sharedFM URLsForDirectory:NSApplicationSupportDirectory
                                             inDomains:NSUserDomainMask];
    NSURL* appSupportDir = nil;
    NSURL* appDirectory = nil;

    if ([possibleURLs count] >= 1) {
        appSupportDir = [possibleURLs objectAtIndex:0];
    }

    // If a valid app support directory exists, add the app's bundle ID to it to specify the final
    // directory.
    if (appSupportDir) {
        NSString* appBundleID = [[NSBundle mainBundle] bundleIdentifier];
        // appDirectory = [appSupportDir URLByAppendingPathComponent:appBundleID];
        appDirectory = [appSupportDir URLByAppendingPathComponent:@"Simple Text"];
    }

    return appDirectory.fileSystemRepresentation;
}
