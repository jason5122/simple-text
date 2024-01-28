#import "LogUtil.h"
#import <os/log.h>

const NSString* bundleId = NSBundle.mainBundle.bundleIdentifier;

void Logv(os_log_type_t type, NSString* category, NSString* format, va_list args) {
    os_log_t osLog = os_log_create([bundleId UTF8String], [category UTF8String]);
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    os_log_with_type(osLog, type, "%{public}@", message);
}

void LogDefault(NSString* category, NSString* format, ...) {
    va_list args;
    va_start(args, format);
    Logv(OS_LOG_TYPE_DEFAULT, category, format, args);
    va_end(args);
}

void LogError(NSString* category, NSString* format, ...) {
    va_list args;
    va_start(args, format);
    Logv(OS_LOG_TYPE_ERROR, category, format, args);
    va_end(args);
}

void LogDefault(std::string category, std::string format, ...) {
    va_list args;
    va_start(args, format);
    Logv(OS_LOG_TYPE_DEFAULT, [NSString stringWithUTF8String:category.c_str()],
         [NSString stringWithUTF8String:format.c_str()], args);
    va_end(args);
}

void LogError(std::string category, std::string format, ...) {
    va_list args;
    va_start(args, format);
    Logv(OS_LOG_TYPE_ERROR, [NSString stringWithUTF8String:category.c_str()],
         [NSString stringWithUTF8String:format.c_str()], args);
    va_end(args);
}
