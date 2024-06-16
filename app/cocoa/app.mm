#include "app/app.h"
#include "app/cocoa/WindowController.h"
#include "app/cocoa/pimpl_mac.h"
#include "app/window.h"
#include <Foundation/Foundation.h>
#include <format>
#include <iostream>
#include <vector>

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    app::App* app;
}

@end

@implementation AppDelegate

- (instancetype)initWithApp:(app::App*)theApp {
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
                                                       action:NULL
                                                keyEquivalent:@""] autorelease];
    fileMenu.submenu = [[[NSMenu alloc] init] autorelease];
    [fileMenu.submenu addItemWithTitle:@"New File" action:@selector(newFile) keyEquivalent:@"n"];
    [fileMenu.submenu addItem:[NSMenuItem separatorItem]];
    [[fileMenu.submenu addItemWithTitle:@"New Window"
                                 action:@selector(newWindow)
                          keyEquivalent:@"n"]
        setKeyEquivalentModifierMask:NSEventModifierFlagShift | NSEventModifierFlagCommand];
    [menu addItem:fileMenu];

    NSApp.mainMenu = menu;

    app->onLaunch();
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    app->onQuit();
}

- (void)showAboutPanel {
    [NSApp orderFrontStandardAboutPanel:menu];
    [NSApp activateIgnoringOtherApps:true];
}

- (void)newFile {
    // If there are no open windows, pass action to `App` instead of `App::Window`.
    if (NSApp.keyWindow == nil) {
        app->onGuiAction(app::GuiAction::kNewFile);
    } else {
        app::Window* app_window = [NSApp.keyWindow.windowController getAppWindow];
        app_window->onGuiAction(app::GuiAction::kNewFile);
    }
}

- (void)newWindow {
    // If there are no open windows, pass action to `App` instead of `App::Window`.
    if (NSApp.keyWindow == nil) {
        app->onGuiAction(app::GuiAction::kNewWindow);
    } else {
        app::Window* app_window = [NSApp.keyWindow.windowController getAppWindow];
        app_window->onGuiAction(app::GuiAction::kNewWindow);
    }
}

- (BOOL)validateMenuItem:(NSMenuItem*)item {
    // if (item.action == @selector(newFile)) {
    //     return NO;
    // }
    // if (item.action == @selector(newWindow)) {
    //     return NO;
    // }
    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)app {
    return YES;
}

@end

namespace app {

App::App() : pimpl{new impl{}} {
    // We must create the application instance once before using `NSApp`.
    [NSApplication sharedApplication];

    AppDelegate* appDelegate = [[[AppDelegate alloc] initWithApp:this] autorelease];

    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    NSApp.delegate = appDelegate;
}

App::~App() {}

void App::run() {
    @autoreleasepool {
        [NSApp run];
    }
}

void App::quit() {
    [NSApp terminate:nil];
}

}
