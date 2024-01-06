#pragma once

#import <Foundation/Foundation.h>
#import <os/log.h>

// https://stackoverflow.com/a/12994075/14698275
#ifdef __cplusplus
extern "C" {
#endif

void logDefault(NSString* category, NSString* format, ...);
void logError(NSString* category, NSString* format, ...);

#ifdef __cplusplus
}
#endif
