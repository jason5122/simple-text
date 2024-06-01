#include "gui/app.h"
#include "gui/cocoa/OpenGLView.h"
#include "gui/cocoa/WindowController.h"
#include "gui/cocoa/pimpl_mac.h"
#include <Foundation/Foundation.h>
#include <format>
#include <iostream>
#include <vector>

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    gui::App* app;
}

@end

@implementation AppDelegate

- (instancetype)initWithApp:(gui::App*)theApp {
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
    [menu addItem:appMenu];

    NSMenuItem* fileMenu = [[[NSMenuItem alloc] initWithTitle:@"File"
                                                       action:nullptr
                                                keyEquivalent:@""] autorelease];
    fileMenu.submenu = [[[NSMenu alloc] init] autorelease];
    [fileMenu.submenu addItemWithTitle:@"New File" action:@selector(newFile) keyEquivalent:@"n"];
    [fileMenu.submenu addItem:[NSMenuItem separatorItem]];
    [[fileMenu.submenu addItemWithTitle:@"New Window"
                                 action:@selector(newWindow)
                          keyEquivalent:@"n"]
        setKeyEquivalentModifierMask:NSEventModifierFlagShift | NSEventModifierFlagCommand];
    [menu addItem:fileMenu];

    NSApplication.sharedApplication.mainMenu = menu;

    app->onLaunch();
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    app->onQuit();
}

- (void)showAboutPanel {
    [NSApplication.sharedApplication orderFrontStandardAboutPanel:menu];
    [NSApplication.sharedApplication activateIgnoringOtherApps:true];
}

- (void)newFile {
    std::cerr << "new file\n";
}

- (void)newWindow {
    std::cerr << "new window\n";
}

- (BOOL)validateMenuItem:(NSMenuItem*)item {
    if (item.action == @selector(newFile)) {
        // return NO;
    }
    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
    return true;
}

@end

namespace gui {

App::App() : pimpl{new impl{}} {
    pimpl->ns_app = NSApplication.sharedApplication;
    AppDelegate* appDelegate = [[[AppDelegate alloc] initWithApp:this] autorelease];

    pimpl->ns_app.activationPolicy = NSApplicationActivationPolicyRegular;
    pimpl->ns_app.delegate = appDelegate;
}

App::~App() {}

void App::run() {
    @autoreleasepool {
        [pimpl->ns_app run];
    }
}

void App::quit() {
    [pimpl->ns_app terminate:nil];
}

}
