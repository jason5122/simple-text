#include "ui/app/app.h"
#include "ui/app/cocoa/OpenGLView.h"
#include "ui/app/cocoa/displaygl.h"
#include <Foundation/Foundation.h>
#include <iostream>
#include <vector>

#import <Cocoa/Cocoa.h>

#include <glad/glad.h>

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
    menu = [[[NSMenu alloc] init] autorelease];
    NSMenuItem* appMenu = [[[NSMenuItem alloc] init] autorelease];
    appMenu.submenu = [[[NSMenu alloc] init] autorelease];
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
    NSPoint cascading_point = NSZeroPoint;
    DisplayGL* displaygl;
};

App::App() : pimpl{new impl{}} {
    pimpl->ns_app = NSApplication.sharedApplication;
    AppDelegate* appDelegate = [[AppDelegate alloc] initWithApp:this];

    pimpl->ns_app.activationPolicy = NSApplicationActivationPolicyRegular;
    pimpl->ns_app.delegate = appDelegate;

    pimpl->displaygl = new DisplayGL();
    pimpl->displaygl->initialize();
}

App::~App() {
    delete pimpl->displaygl;
}

void App::run() {
    @autoreleasepool {
        [pimpl->ns_app run];
    }
}

// This isn't required for Cocoa.
void App::incrementWindowCount() {}

class App::Window::impl {
public:
    NSWindow* ns_window;
    OpenGLView* opengl_view;
};

App::Window::Window(App& parent, int width, int height) : pimpl{new impl{}}, parent(parent) {
    NSRect frame = NSMakeRect(500, 0, width, height);
    NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                             NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    pimpl->ns_window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:false];
    pimpl->ns_window.title = @"Simple Text";
    pimpl->opengl_view = [[[OpenGLView alloc] initWithFrame:frame
                                                  appWindow:this
                                                  displaygl:parent.pimpl->displaygl] autorelease];
    pimpl->ns_window.contentView = pimpl->opengl_view;
    [pimpl->ns_window makeFirstResponder:pimpl->opengl_view];

    // Bypass the user's tabbing preference.
    // https://stackoverflow.com/a/40826761/14698275
    pimpl->ns_window.tabbingMode = NSWindowTabbingModeDisallowed;

    // Implement window cascading.
    // if (NSEqualPoints(parent.pimpl->cascading_point, NSZeroPoint)) {
    //     NSPoint point = pimpl->ns_window.frame.origin;
    //     parent.pimpl->cascading_point = [pimpl->ns_window cascadeTopLeftFromPoint:point];

    //     [pimpl->ns_window center];
    // } else {
    //     parent.pimpl->cascading_point =
    //         [pimpl->ns_window cascadeTopLeftFromPoint:parent.pimpl->cascading_point];
    // }
}

App::Window::~Window() {}

void App::Window::show() {
    [pimpl->ns_window makeKeyAndOrderFront:nil];
}

void App::Window::close() {
    [pimpl->ns_window close];
}

void App::Window::redraw() {
    [pimpl->opengl_view redraw];
}
