#include "ui/app/app.h"
#include "ui/app/cocoa/OpenGLView.h"
#include <iostream>
#include <vector>

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSMenu* menu;
    Parent* app;
}

@end

@implementation AppDelegate

- (instancetype)initWithApp:(Parent*)theApp {
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

class Parent::impl {
public:
    NSApplication* ns_app;
};

Parent::Parent() : pimpl{new impl{}} {
    pimpl->ns_app = NSApplication.sharedApplication;
    AppDelegate* appDelegate = [[AppDelegate alloc] initWithApp:this];

    pimpl->ns_app.activationPolicy = NSApplicationActivationPolicyRegular;
    pimpl->ns_app.delegate = appDelegate;
}

Parent::~Parent() {}

void Parent::run() {
    @autoreleasepool {
        [pimpl->ns_app run];
    }
}

Parent::Child* Parent::createChild() {
    Child* child = new Child(*this);
    child->create(600, 400);

    return child;
}

void Parent::destroyChild(Child* child) {
    if (!child) return;

    child->destroy();
    delete child;
}

class Parent::Child::impl {
public:
    NSWindow* ns_window;
};

Parent::Child::Child(Parent& parent) : pimpl{new impl{}}, parent(parent), ram_waster(5000000, 1) {}

Parent::Child::~Child() {}

void Parent::Child::create(int width, int height) {
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
    [pimpl->ns_window makeKeyAndOrderFront:nil];
}

void Parent::Child::destroy() {
    [pimpl->ns_window close];
}

void Parent::Child::onKeyDownVirtual(app::Key key, app::ModifierKey modifiers) {
    if (key == app::Key::kN && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.createChild();
    }
    if (key == app::Key::kW && modifiers == (app::kPrimaryModifier | app::ModifierKey::kShift)) {
        parent.destroyChild(this);
    }
}
