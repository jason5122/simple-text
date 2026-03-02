#include "app/cocoa/app_controller.h"
#include "base/check.h"
#include <print>

@implementation AppController

+ (AppController*)sharedController {
    static AppController* sharedController = [] {
        AppController* sharedController = [[AppController alloc] init];
        NSApp.delegate = sharedController;
        return sharedController;
    }();

    CHECK_NE(nil, sharedController);
    CHECK_EQ(NSApp.delegate, sharedController);
    return sharedController;
}

// This is the Apple-approved place to override the default handlers.
- (void)applicationWillFinishLaunching:(NSNotification*)notification {
    // TODO: We create windows before [NSApp run], so setting this here is too late. Find out how
    // Chromium creates windows after [NSApp run].
    // NSWindow.allowsAutomaticWindowTabbing = NO;
}

// Called when the dock icon is clicked. We should open a new window if there are no windows.
- (BOOL)applicationShouldHandleReopen:(NSApplication*)sender
                    hasVisibleWindows:(BOOL)hasVisibleWindows {
    std::println("reopen: hasVisibleWindows = {}", hasVisibleWindows);
    return NO;
}

// https://sector7.computest.nl/post/2022-08-process-injection-breaking-all-macos-security-layers-with-a-single-vulnerability/
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
    return YES;
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
    // TODO: Implement.
    return YES;
}

@end
