#pragma once

#ifdef __OBJC__
#import <Foundation/Foundation.h>
void LogDefault(NSString* category, NSString* format, ...);
void LogError(NSString* category, NSString* format, ...);
#endif

void LogDefault(const char* category, const char* format, ...);
void LogError(const char* category, const char* format, ...);
