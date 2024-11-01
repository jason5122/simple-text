#include "app/app.h"
#include "app/cocoa/impl_cocoa.h"
#include "app/cocoa/window_controller.h"
#include "app/window.h"
#include <Foundation/Foundation.h>
#include <format>
#include <vector>

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    app::App* app;
}

- (void)callAppAction:(app::AppAction)appAction;

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

- (void)callAppAction:(app::AppAction)appAction {
    // If there are no open windows, pass action to `App` instead of `App::Window`.
    if (NSApp.keyWindow == nil) {
        app->onAppAction(appAction);
    } else {
        app::Window* app_window = [NSApp.keyWindow.windowController getAppWindow];

        // `app_window` is null if we are in the about panel.
        // TODO: Create our own "About ..." panel.
        if (app_window) {
            app_window->onAppAction(appAction);
        } else {
            app->onAppAction(appAction);
        }
    }
}

- (void)newFile {
    [self callAppAction:app::AppAction::kNewFile];
}

- (void)newWindow {
    [self callAppAction:app::AppAction::kNewWindow];
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

std::string App::getClipboardString() {
    NSString* ns_string = [NSPasteboard.generalPasteboard stringForType:NSPasteboardTypeString];
    std::string str(ns_string.UTF8String);
    return str;
}

void App::setClipboardString(const std::string& str8) {
    NSPasteboard* pboard = NSPasteboard.generalPasteboard;
    NSString* ns_string = [[NSString alloc] initWithUTF8String:str8.data()];
    [pboard clearContents];
    [pboard setString:ns_string forType:NSPasteboardTypeString];
}

void App::setCursorStyle(CursorStyle style) {
    if (style == CursorStyle::kArrow) {
        [NSCursor.arrowCursor set];
    } else if (style == CursorStyle::kIBeam) {
        [NSCursor.IBeamCursor set];
    }
}

}  // namespace app
