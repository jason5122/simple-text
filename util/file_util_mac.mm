#import "file_util.h"
#import <Foundation/Foundation.h>

fs::path ResourcePath() {
    return NSBundle.mainBundle.resourcePath.fileSystemRepresentation;
}
