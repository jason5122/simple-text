#include "ui/app/app.h"
#include "ui/app/cocoa/OpenGLView.h"
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
    [menu addItem:appMenu];
    NSApplication.sharedApplication.mainMenu = menu;

    app->onActivate();
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

void App::run() {
    @autoreleasepool {
        [pimpl->ns_app run];
    }
}

void App::createNewWindow(AppWindow& app_window, int width, int height) {
    NSRect frame = NSMakeRect(0, 0, width, height);

    NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                             NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    NSWindow* ns_window = [[NSWindow alloc] initWithContentRect:frame
                                                      styleMask:mask
                                                        backing:NSBackingStoreBuffered
                                                          defer:false];
    ns_window.title = @"Simple Text";

    // Bypass the user's tabbing preference.
    // https://stackoverflow.com/a/40826761/14698275
    ns_window.tabbingMode = NSWindowTabbingModeDisallowed;

    OpenGLView* opengl_view = [[OpenGLView alloc] initWithFrame:frame appWindow:app_window];
    ns_window.contentView = opengl_view;
    [ns_window makeFirstResponder:opengl_view];

    [ns_window center];
    [ns_window makeKeyAndOrderFront:nil];
}

App::~App() {}

class App::Window::impl {};

App::Window::Window(App& app) : parent(app), pimpl{new impl{}} {}

void App::Window::createWithSize(int width, int height) {
    NSRect frame = NSMakeRect(0, 0, width, height);

    NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                             NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    NSWindow* ns_window = [[NSWindow alloc] initWithContentRect:frame
                                                      styleMask:mask
                                                        backing:NSBackingStoreBuffered
                                                          defer:false];
    ns_window.title = @"Simple Text";

    // Bypass the user's tabbing preference.
    // https://stackoverflow.com/a/40826761/14698275
    ns_window.tabbingMode = NSWindowTabbingModeDisallowed;

    // OpenGLView* opengl_view = [[OpenGLView alloc] initWithFrame:frame appWindow:app_window];
    // ns_window.contentView = opengl_view;
    // [ns_window makeFirstResponder:opengl_view];

    [ns_window center];
    [ns_window makeKeyAndOrderFront:nil];
}
