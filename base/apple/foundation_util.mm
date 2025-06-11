#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"

namespace base::apple {

NSString* FilePathToNSString(const FilePath& path) {
    return static_cast<NSString*>(FilePathToCFString(path).release());
}

FilePath NSStringToFilePath(NSString* str) {
    return CFStringToFilePath(static_cast<CFStringRef>(str));
}

ScopedCFTypeRef<CFStringRef> FilePathToCFString(const FilePath& path) {
    if (path.empty()) {
        return ScopedCFTypeRef<CFStringRef>();
    }

    return ScopedCFTypeRef<CFStringRef>(
        CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, path.value().c_str()));
}

FilePath CFStringToFilePath(CFStringRef str) {
    if (!str || CFStringGetLength(str) == 0) {
        return FilePath();
    }

    return FilePath(FilePath::GetHFSDecomposedForm(str));
}

}  // namespace base::apple
