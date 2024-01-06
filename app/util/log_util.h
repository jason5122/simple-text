#pragma once

#import <Foundation/Foundation.h>
#import <os/log.h>

void custom_log(os_log_type_t type, NSString* category, NSString* format, ...);
