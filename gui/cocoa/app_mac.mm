#include "gui/app.h"
#include "gui/cocoa/OpenGLView.h"
#include "gui/cocoa/WindowController.h"
#include "gui/cocoa/displaygl.h"
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
    AppDelegate* appDelegate = [[[AppDelegate alloc] initWithApp:this] autorelease];

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

void App::quit() {
    [pimpl->ns_app terminate:nil];
}

class App::Window::impl {
public:
    WindowController* window_controller;
};

App::Window::Window(App& parent, int width, int height) : pimpl{new impl{}}, parent(parent) {
    // NSRect frame = NSMakeRect(500, 0, width, height);
    NSRect frame = NSScreen.mainScreen.visibleFrame;

    std::cerr << "scale factor: " << NSScreen.mainScreen.backingScaleFactor << '\n';

    pimpl->window_controller = [[WindowController alloc] initWithFrame:frame
                                                             appWindow:this
                                                             displayGl:parent.pimpl->displaygl];

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

App::Window::~Window() {
    [pimpl->window_controller release];
}

void App::Window::show() {
    [pimpl->window_controller show];
}

void App::Window::close() {
    [pimpl->window_controller close];
}

void App::Window::redraw() {
    [pimpl->window_controller redraw];
}

int App::Window::width() {
    return [pimpl->window_controller getWidth];
}

int App::Window::height() {
    return [pimpl->window_controller getHeight];
}

int App::Window::scaleFactor() {
    return [pimpl->window_controller getScaleFactor];
}

bool App::Window::isDarkMode() {
    return [pimpl->window_controller isDarkMode];
}
