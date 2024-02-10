#import "file_util.h"
#import <Foundation/Foundation.h>

std::filesystem::path ResourcePath() {
    return NSBundle.mainBundle.resourcePath.fileSystemRepresentation;
}
