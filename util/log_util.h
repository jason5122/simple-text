#pragma once

#ifdef __OBJC__
#import <Foundation/Foundation.h>
void LogDefault(NSString* category, NSString* format, ...);
void LogError(NSString* category, NSString* format, ...);
#endif

#ifdef __cplusplus
#include <string>
void LogDefault(std::string category, std::string format, ...);
void LogError(std::string category, std::string format, ...);
#endif

#ifdef __cplusplus
extern "C" {
#endif

void LogDefault(const char* category, const char* format, ...);
void LogError(const char* category, const char* format, ...);

#ifdef __cplusplus
}
#endif
