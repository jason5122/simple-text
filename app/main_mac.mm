#import "AppDelegate.h"
#import <Cocoa/Cocoa.h>

int SimpleTextMain() {
    @autoreleasepool {
        NSApplication* app = NSApplication.sharedApplication;
        AppDelegate* appDelegate = [[AppDelegate alloc] init];

        app.activationPolicy = NSApplicationActivationPolicyRegular;
        app.delegate = appDelegate;

        [app run];
    }
    return 0;
}
