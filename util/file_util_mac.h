#pragma once

// https://stackoverflow.com/a/12994075/14698275
#ifdef __cplusplus
extern "C" {
#endif

const char* ResourcePath(const char* resource_name);
char* ReadFile(const char* file_name);

#ifdef __cplusplus
}
#endif
