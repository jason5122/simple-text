#import "FileUtil.h"
#import <Foundation/Foundation.h>
#import <sys/stat.h>

const char* resourcePath(const char* resourceName) {
    NSString* path =
        [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:resourceName]
                                        ofType:nil];
    return [path fileSystemRepresentation];
}

char* readFile(const char* fileName) {
    struct stat statbuf;
    FILE* fh;
    char* source;

    fh = fopen(fileName, "r");
    if (fh == 0) return 0;

    stat(fileName, &statbuf);
    source = (char*)malloc(statbuf.st_size + 1);
    fread(source, statbuf.st_size, 1, fh);
    source[statbuf.st_size] = '\0';
    fclose(fh);

    return source;
}
