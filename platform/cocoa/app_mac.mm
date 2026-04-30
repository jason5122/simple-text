#include "base/check.h"
#include "platform/cocoa/app_mac.h"
#include "platform/cocoa/window_mac.h"
#include <Cocoa/Cocoa.h>
#include <algorithm>

@interface PlatformAppDelegate : NSObject <NSApplicationDelegate> {
@public
    void* app;
}
@end

@implementation PlatformAppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication*)sender {
    return YES;
}

@end

namespace {

void build_main_menu() {
    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@""];
    [app_menu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"]];
    app_item.submenu = app_menu;
    [main_menu addItem:app_item];
    NSApp.mainMenu = main_menu;
}

}  // namespace

namespace platform {

AppMac::AppMac(AppOptions options) : options_(options) {}

AppMac::~AppMac() {
    if (NSApp.delegate == (__bridge id)app_delegate_) {
        NSApp.delegate = nil;
    }
}

std::unique_ptr<App> AppMac::create(AppOptions options) {
    CHECK_EQ(RendererBackend::kOpenGL, options.renderer_backend);

    [NSApplication sharedApplication];
    NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
    NSWindow.allowsAutomaticWindowTabbing = NO;
    build_main_menu();

    auto app = std::unique_ptr<AppMac>(new AppMac(options));
    PlatformAppDelegate* delegate = [PlatformAppDelegate new];
    delegate->app = app.get();
    app->app_delegate_ = (__bridge void*)delegate;
    NSApp.delegate = delegate;
    return app;
}

Window* AppMac::create_window(WindowOptions options, WindowDelegate* delegate) {
    auto window = WindowMac::create(*this, std::move(options), delegate);
    Window* raw_window = window.get();
    windows_.push_back(std::move(window));
    return raw_window;
}

int AppMac::run() {
    [NSApp run];
    return 0;
}

void AppMac::quit() { [NSApp terminate:nil]; }

void AppMac::window_did_close(WindowMac* window) {
    std::erase_if(windows_, [window](const std::unique_ptr<WindowMac>& candidate) {
        return candidate.get() == window;
    });
}

}  // namespace platform
