#import "file_util_mac.h"
#import <Foundation/Foundation.h>

const char* ResourcePath(std::string resource_name) {
    NSString* path =
        [NSBundle.mainBundle pathForResource:[NSString stringWithUTF8String:resource_name.c_str()]
                                      ofType:nil];
    return path.fileSystemRepresentation;
}
