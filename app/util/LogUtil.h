#pragma once

#import <Foundation/Foundation.h>
#import <os/log.h>

void logDefault(NSString* category, NSString* format, ...);
void logError(NSString* category, NSString* format, ...);
