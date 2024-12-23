#include "app/app.h"

#include "app/cocoa/impl_cocoa.h"
#include "app/cocoa/window_controller.h"
#include "app/window.h"

#include <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h>
#include <vector>

// TODO: Debug use; remove this.
#include <fmt/base.h>

// Taken from `//chromium/src/chrome/browser/ui/cocoa/main_menu_builder.mm.
namespace {
inline NSMenuItem* CreateMenuItem(NSString* title, SEL action, NSString* key_equivalent) {
    return [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key_equivalent];
}

NSMenu* BuildAppMenu() {
    // The title is not used, as the title will always be the name of the app.
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@""];

    NSMenuItem* item;
    item = CreateMenuItem(@"About Simple Text", @selector(orderFrontStandardAboutPanel:), @"");
    [menu addItem:item];

    [menu addItem:NSMenuItem.separatorItem];

    item = CreateMenuItem(@"Hide Simple Text", @selector(hide:), @"h");
    [menu addItem:item];

    item = CreateMenuItem(@"Hide Others", @selector(hideOtherApplications:), @"h");
    item.keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
    [menu addItem:item];

    item = CreateMenuItem(@"Show All", @selector(unhideAllApplications:), @"");
    [menu addItem:item];

    [menu addItem:NSMenuItem.separatorItem];

    item = CreateMenuItem(@"Quit Simple Text", @selector(terminate:), @"q");
    [menu addItem:item];

    return menu;
}

NSMenu* BuildEditMenu() {
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Edit"];

    NSMenuItem* item;
    item = CreateMenuItem(@"Emoji & Symbols", @selector(orderFrontCharacterPalette:), @"e");
    item.keyEquivalentModifierMask = NSEventModifierFlagFunction;
    [menu addItem:item];

    return menu;
}

NSMenu* BuildViewMenu() {
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"View"];

    NSMenuItem* item;
    // The title is not used, as the title will always become "Enter Full Screen".
    // This entry is part of `NSApp.windowsMenu` by default, but defining it elsewhere will
    // actually prevent it from showing up by default.
    item = CreateMenuItem(@"", @selector(toggleFullScreen:), @"f");
    item.keyEquivalentModifierMask = NSEventModifierFlagFunction;
    [menu addItem:item];
    NSApp.helpMenu = menu;
    return menu;
}

NSMenu* BuildWindowMenu() {
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Window"];

    NSMenuItem* item;
    item = CreateMenuItem(@"Minimize", @selector(performMiniaturize:), @"m");
    [menu addItem:item];
    item = CreateMenuItem(@"Zoom", @selector(performZoom:), @"");
    [menu addItem:item];
    item = CreateMenuItem(@"Bring All To Front", @selector(arrangeInFront:), @"");
    [menu addItem:item];
    NSApp.windowsMenu = menu;
    return menu;
}

NSMenu* BuildHelpMenu() {
    NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Help"];

    NSMenuItem* item;
    item = CreateMenuItem(@"Simple Text Help", @selector(showHelp:), @"");
    [menu addItem:item];
    NSApp.helpMenu = menu;
    return menu;
}

void BuildMainMenu() {
    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];

    using Builder = NSMenu* (*)();
    static const Builder kBuilderFuncs[] = {&BuildAppMenu, &BuildEditMenu, &BuildViewMenu,
                                            &BuildWindowMenu, &BuildHelpMenu};
    for (auto* builder : kBuilderFuncs) {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        item.submenu = builder();
        [main_menu addItem:item];
    }
    NSApp.mainMenu = main_menu;
}
}  // namespace

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    app::App* app;
}

// TODO: Refactor this.
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
    NSWindow.allowsAutomaticWindowTabbing = NO;

    BuildMainMenu();

    app->onLaunch();
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    app->onQuit();
}

- (void)callAppAction:(app::AppAction)appAction {
    // If there are no open windows, pass action to `App` instead of `App::Window`.
    if (NSApp.keyWindow == nil) {
        app->onAppAction(appAction);
    } else {
        WindowController* wc = NSApp.keyWindow.windowController;
        app::Window* app_window = wc.appWindow;

        // `app_window` is null if we are in the about panel.
        // TODO: Create our own "About ..." panel.
        if (app_window) {
            app_window->onAppAction(appAction);
        } else {
            app->onAppAction(appAction);
        }
    }
}

// TODO: Refactor this.
- (void)newFile {
    [self callAppAction:app::AppAction::kNewFile];
}

// TODO: Refactor this.
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

    AppDelegate* app_delegate = [[[AppDelegate alloc] initWithApp:this] autorelease];
    NSApp.delegate = app_delegate;
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
