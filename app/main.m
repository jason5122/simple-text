#import "AppDelegate.h"
#import <Cocoa/Cocoa.h>

int main() {
    @autoreleasepool {
        NSApplication* app = NSApplication.sharedApplication;
        AppDelegate* appDelegate = [[AppDelegate alloc] init];

        app.activationPolicy = NSApplicationActivationPolicyRegular;
        app.delegate = appDelegate;

        [app run];
    }
}
