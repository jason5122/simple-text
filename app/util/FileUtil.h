#pragma once

// https://stackoverflow.com/a/12994075/14698275
#ifdef __cplusplus
extern "C" {
#endif

const char* resourcePath(const char* resourceName);
char* readFile(const char* fileName);

#ifdef __cplusplus
}
#endif
