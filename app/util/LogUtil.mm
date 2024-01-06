#import "LogUtil.h"

const NSString* bundleId = NSBundle.mainBundle.bundleIdentifier;

void logv(os_log_type_t type, NSString* category, NSString* format, va_list args) {
    os_log_t osLog = os_log_create([bundleId UTF8String], [category UTF8String]);
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    os_log_with_type(osLog, type, "%{public}@", message);
}

void logDefault(NSString* category, NSString* format, ...) {
    va_list args;
    va_start(args, format);
    logv(OS_LOG_TYPE_DEFAULT, category, format, args);
    va_end(args);
}

void logError(NSString* category, NSString* format, ...) {
    va_list args;
    va_start(args, format);
    logv(OS_LOG_TYPE_ERROR, category, format, args);
    va_end(args);
}
