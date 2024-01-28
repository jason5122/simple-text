#pragma once

#ifdef __OBJC__
#import <Foundation/Foundation.h>
void LogDefault(NSString* category, NSString* format, ...);
void LogError(NSString* category, NSString* format, ...);
#endif

#include <string>

void LogDefault(std::string category, std::string format, ...);
void LogError(std::string category, std::string format, ...);
