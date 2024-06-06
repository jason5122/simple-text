#include "ui/app/app.h"
#include "ui/app/cocoa/WindowController.h"
#include "util/profile_util.h"
#include <vector>

#import <Cocoa/Cocoa.h>

#include <format>
#include <iostream>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    App* app;
}

@end

@implementation AppDelegate

- (instancetype)initWithApp:(App*)theApp {
    self = [super init];
    if (self) {
        self->app = theApp;
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

    {
        PROFILE_BLOCK("app->onActivate()");
        app->onActivate();
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

class App::impl {
public:
    std::vector<WindowController*> window_controllers;
};

App::App() : pimpl{new impl{}}, launch_time(std::chrono::high_resolution_clock::now()) {
    {
        PROFILE_BLOCK("[NSApplication sharedApplication]");
        [NSApplication sharedApplication];
    }

    AppDelegate* appDelegate = [[AppDelegate alloc] initWithApp:this];
    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    NSApp.delegate = appDelegate;
}

void App::run() {
    @autoreleasepool {
        [NSApp run];
    }
}

void App::createNewWindow() {
    NSRect frame = NSMakeRect(0, 0, 1200, 800);
    WindowController* window_controller = [[WindowController alloc] initWithFrame:frame];

    [window_controller showWindow];

    // pimpl->window_controllers.push_back(window_controller);

    // TODO: For debugging; remove this.
    if (!has_drawn) {
        has_drawn = true;

        auto draw_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(draw_time - launch_time).count();
        std::cerr << std::format("startup time: {} µs", duration) << '\n';
    }
}

App::~App() {}
