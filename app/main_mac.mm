#import "ui/cocoa/WindowController.h"
#import "util/profile_util.h"
#import <Cocoa/Cocoa.h>

#include <chrono>
#include <format>
#include <iostream>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    WindowController* windowController;
    NSMenu* menu;
    bool has_drawn;
    std::chrono::high_resolution_clock::time_point launch_time;
}

@end

@implementation AppDelegate

- (instancetype)init:(std::chrono::high_resolution_clock::time_point)theLaunchTime {
    has_drawn = false;
    // launch_time = std::chrono::high_resolution_clock::now();
    launch_time = theLaunchTime;

    self = [super init];
    if (self) {
        NSRect frameRect = NSMakeRect(0, 0, 600, 500);
        {
            // ~12000-13000 µs
            PROFILE_BLOCK("[WindowController alloc]");
            windowController = [[WindowController alloc] initWithFrame:frameRect];
        }
    }
    return self;
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
    NSString* appName = NSBundle.mainBundle.infoDictionary[@"CFBundleName"];
    menu = [[NSMenu alloc] init];
    NSMenuItem* appMenu = [[NSMenuItem alloc] init];
    appMenu.submenu = [[NSMenu alloc] init];
    [appMenu.submenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName]
                               action:@selector(showAboutPanel)
                        keyEquivalent:@""];
    [appMenu.submenu addItem:[NSMenuItem separatorItem]];
    [appMenu.submenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
                               action:@selector(terminate:)
                        keyEquivalent:@"q"];
    [menu addItem:appMenu];
    NSApplication.sharedApplication.mainMenu = menu;

    [windowController showWindow];

    // TODO: For debugging; remove this.
    if (!has_drawn) {
        has_drawn = true;

        auto draw_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(draw_time - launch_time).count();
        std::cerr << std::format("startup time: {} µs", duration) << '\n';
    }
}

- (void)showAboutPanel {
    [NSApplication.sharedApplication orderFrontStandardAboutPanel:menu];
    [NSApplication.sharedApplication activateIgnoringOtherApps:true];
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
    return true;
}

@end

int SimpleTextMain(int argc, char* argv[]) {
    @autoreleasepool {
        auto launch_time = std::chrono::high_resolution_clock::now();

        [NSApplication sharedApplication];

        AppDelegate* appDelegate = [[AppDelegate alloc] init:launch_time];

        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
        NSApp.delegate = appDelegate;

        [NSApp run];
    }
    return 0;
}
