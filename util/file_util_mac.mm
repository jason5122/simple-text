#import "file_util.h"
#import <Foundation/Foundation.h>
#import <sys/stat.h>

const char* ResourcePath(const char* resource_name) {
    NSString* path =
        [NSBundle.mainBundle pathForResource:[NSString stringWithUTF8String:resource_name]
                                      ofType:nil];
    return path.fileSystemRepresentation;
}

char* ReadFile(const char* file_name) {
    struct stat statbuf;
    FILE* fh;
    char* source;

    fh = fopen(file_name, "r");
    if (fh == 0) return 0;

    stat(file_name, &statbuf);
    source = (char*)malloc(statbuf.st_size + 1);
    fread(source, statbuf.st_size, 1, fh);
    source[statbuf.st_size] = '\0';
    fclose(fh);

    return source;
}
