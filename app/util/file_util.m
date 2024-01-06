#import <Foundation/Foundation.h>
#import <sys/stat.h>

const char* pathForResource(const char* name) {
    NSString* path = [[NSBundle mainBundle]
        pathForResource:[NSString stringWithUTF8String:name]
                 ofType:nil];
    return [path fileSystemRepresentation];
}

char* readFile(const char* name) {
    struct stat statbuf;
    FILE* fh;
    char* source;

    fh = fopen(name, "r");
    if (fh == 0) return 0;

    stat(name, &statbuf);
    source = (char*)malloc(statbuf.st_size + 1);
    fread(source, statbuf.st_size, 1, fh);
    source[statbuf.st_size] = '\0';
    fclose(fh);

    return source;
}
