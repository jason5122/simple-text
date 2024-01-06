#import "log_util.h"

const NSString* bundleId = NSBundle.mainBundle.bundleIdentifier;

void custom_logv(os_log_type_t type, NSString* category, NSString* format, va_list args) {
    os_log_t customLog = os_log_create([bundleId UTF8String], [category UTF8String]);
    NSString* message = [[NSString alloc] initWithFormat:format arguments:args];
    os_log_with_type(customLog, type, "%{public}@", message);
}

void custom_log(os_log_type_t type, NSString* category, NSString* format, ...) {
    va_list args;
    va_start(args, format);
    custom_logv(type, category, format, args);
    va_end(args);
}
