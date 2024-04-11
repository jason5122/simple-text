#include "ui/app/app.h"
#include "ui/app/cocoa/OpenGLView.h"
#include <iostream>
#include <vector>

#import <Cocoa/Cocoa.h>

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
    // [appMenu.submenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
    //                            action:@selector(terminate:)
    //                     keyEquivalent:@""];
    [menu addItem:appMenu];
    NSApplication.sharedApplication.mainMenu = menu;

    app->onLaunch();
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
    NSApplication* ns_app;
};

App::App() : pimpl{new impl{}} {
    pimpl->ns_app = NSApplication.sharedApplication;
    AppDelegate* appDelegate = [[AppDelegate alloc] initWithApp:this];

    pimpl->ns_app.activationPolicy = NSApplicationActivationPolicyRegular;
    pimpl->ns_app.delegate = appDelegate;
}

App::~App() {}

void App::run() {
    @autoreleasepool {
        [pimpl->ns_app run];
    }
}

class App::Window::impl {
public:
    NSWindow* ns_window;
};

App::Window::Window(App& parent, int width, int height)
    : pimpl{new impl{}}, parent(parent), ram_waster(5000000, 1) {
    NSRect frame = NSMakeRect(0, 0, width, height);
    NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                             NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    pimpl->ns_window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:false];
    pimpl->ns_window.title = @"Simple Text";
    OpenGLView* view = [[[OpenGLView alloc] initWithFrame:frame appWindow:this] autorelease];
    pimpl->ns_window.contentView = view;
    [pimpl->ns_window makeFirstResponder:view];
}

App::Window::~Window() {}

void App::Window::show() {
    [pimpl->ns_window makeKeyAndOrderFront:nil];
}

void App::Window::close() {
    [pimpl->ns_window close];
}
