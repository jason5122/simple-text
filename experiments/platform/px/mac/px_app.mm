// Process-level half of the Cocoa backend: PXApplication, PXApplicationDelegate, the event loop,
// timers, and the small odds and ends of the flat API that are not per-window.
//
// ST's shape, from the binary: PXApplication overrides exactly one method (terminate:), and
// PXApplicationDelegate implements application:openFile:, application:openFiles:,
// applicationDidFinishLaunching:, applicationDockMenu:, applicationOpenUntitledFile:,
// applicationShouldTerminate:, osAppearanceChanged: and scrollBarStyleChanged:. Each one adapts an
// AppKit callback onto px_application_event_handler and does nothing else.

#include "experiments/platform/px/mac/px_mac_private.h"

#include <chrono>
#include <string>
#include <vector>

namespace {

px_application_event_handler* g_app_handler = nullptr;
px_application_event_handler g_default_app_handler;

std::chrono::steady_clock::time_point g_start;

px_application_event_handler& app_handler() {
    return g_app_handler ? *g_app_handler : g_default_app_handler;
}

// Reused for application:openFiles: and the dock's "open with".
struct PathBuffer {
    std::vector<std::string> storage;
    std::vector<const char*> pointers;

    void assign(NSArray<NSString*>* paths) {
        storage.clear();
        pointers.clear();
        for (NSString* path in paths) {
            storage.emplace_back(path.UTF8String);
        }
        pointers.reserve(storage.size());
        for (const std::string& s : storage) {
            pointers.push_back(s.c_str());
        }
    }
};

void build_main_menu(const char* app_name) {
    NSString* name = app_name ? @(app_name) : @"px";

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [main_menu addItem:app_item];

    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:name];
    [app_menu addItemWithTitle:[@"About " stringByAppendingString:name]
                        action:@selector(orderFrontStandardAboutPanel:)
                 keyEquivalent:@""];
    [app_menu addItem:[NSMenuItem separatorItem]];
    [app_menu addItemWithTitle:[@"Hide " stringByAppendingString:name]
                        action:@selector(hide:)
                 keyEquivalent:@"h"];
    [app_menu addItemWithTitle:@"Hide Others"
                        action:@selector(hideOtherApplications:)
                 keyEquivalent:@""];
    [app_menu addItem:[NSMenuItem separatorItem]];
    [app_menu addItemWithTitle:[@"Quit " stringByAppendingString:name]
                        action:@selector(terminate:)
                 keyEquivalent:@"q"];
    app_item.submenu = app_menu;

    NSApp.mainMenu = main_menu;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// PXApplication
// ─────────────────────────────────────────────────────────────────────────────────────────────────

@interface PXApplication : NSApplication
@end

@implementation PXApplication

// The only override ST makes. Quit has to be interceptable, or an editor cannot prompt about
// unsaved buffers; routing through applicationShouldTerminate: keeps the decision in one place.
- (void)terminate:(id)sender {
    if ([self.delegate respondsToSelector:@selector(applicationShouldTerminate:)]) {
        const NSApplicationTerminateReply reply = [self.delegate applicationShouldTerminate:self];
        if (reply == NSTerminateCancel) {
            return;
        }
        if (reply == NSTerminateLater) {
            return;  // The handler calls replyToApplicationShouldTerminate: when it knows.
        }
    }
    [super terminate:sender];
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// PXApplicationDelegate
// ─────────────────────────────────────────────────────────────────────────────────────────────────

@interface PXApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@implementation PXApplicationDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    [NSApp activateIgnoringOtherApps:YES];

    [NSDistributedNotificationCenter.defaultCenter
        addObserver:self
           selector:@selector(osAppearanceChanged:)
               name:@"AppleInterfaceThemeChangedNotification"
             object:nil];
}

- (BOOL)application:(NSApplication*)application openFile:(NSString*)filename {
    (void)application;
    const char* path = filename.UTF8String;
    app_handler().open_files(&path, 1);
    return YES;
}

- (void)application:(NSApplication*)application openFiles:(NSArray<NSString*>*)filenames {
    (void)application;
    PathBuffer buffer;
    buffer.assign(filenames);
    app_handler().open_files(buffer.pointers.empty() ? nullptr : buffer.pointers.data(),
                             static_cast<int>(buffer.pointers.size()));
    [application replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (BOOL)applicationOpenUntitledFile:(NSApplication*)application {
    (void)application;
    app_handler().new_file();
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)application {
    (void)application;
    if (app_handler().can_quit_without_prompt()) {
        return NSTerminateNow;
    }
    app_handler().try_quit([](bool should_quit) {
        [NSApp replyToApplicationShouldTerminate:should_quit ? YES : NO];
    });
    return NSTerminateLater;
}

- (void)osAppearanceChanged:(NSNotification*)notification {
    (void)notification;
    // The notification arrives before NSApp.effectiveAppearance updates, so ST bounces through the
    // main queue before reading it back. Same trick here.
    dispatch_async(dispatch_get_main_queue(), ^{
      app_handler().appearance_changed();
    });
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)application {
    (void)application;
    return YES;
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FLAT API: PROCESS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

namespace {
PXApplicationDelegate* g_app_delegate = nil;
}

void px_init(const char* app_name, const char* bundle_id, int argc, char** argv, uint32_t flags) {
    (void)bundle_id;
    (void)argc;
    (void)argv;
    (void)flags;

    g_start = std::chrono::steady_clock::now();

    // Instantiating the shared application through our subclass is what makes NSApp a
    // PXApplication; there is no Info.plist NSPrincipalClass in play here.
    [PXApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    px_mac_install_before_waiting_observer();

    g_app_delegate = [[PXApplicationDelegate alloc] init];
    NSApp.delegate = g_app_delegate;

    build_main_menu(app_name);

    // Windows are constructed and shown before px_run_event_loop(). Finish AppKit launch now so
    // activation requests made by px_show_window() are honored by Launch Services instead of being
    // silently ignored until -run. Calling -run after an explicit -finishLaunching is supported
    // and simply enters the already-initialised application event loop.
    [NSApp finishLaunching];
}

void px_set_application_event_handler(px_application_event_handler* handler) {
    g_app_handler = handler;
}

void px_run_event_loop() { [NSApp run]; }

void px_exit_event_loop() {
    [NSApp stop:nil];
    // -[NSApplication stop:] only takes effect once the loop processes another event, so post a
    // no-op to wake it immediately.
    NSEvent* wake = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                       location:NSZeroPoint
                                  modifierFlags:0
                                      timestamp:0
                                   windowNumber:0
                                        context:nil
                                        subtype:0
                                          data1:0
                                          data2:0];
    [NSApp postEvent:wake atStart:YES];
}

void px_set_timeout(std::function<void()> fn, int milliseconds) {
    __block std::function<void()> callback = std::move(fn);
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(milliseconds) * NSEC_PER_MSEC),
        dispatch_get_main_queue(), ^{
          callback();
        });
}

bool px_os_in_dark_mode() {
    NSAppearanceName name = [NSApp.effectiveAppearance
        bestMatchFromAppearancesWithNames:@[ NSAppearanceNameAqua, NSAppearanceNameDarkAqua ]];
    return [name isEqualToString:NSAppearanceNameDarkAqua];
}

double px_caret_blink_time() {
    // Cached on first read, matching the px_caret_blink_time()::blink_time static in ST.
    static const double blink_time = [] {
        NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
        const double on_ms = [defaults doubleForKey:@"NSTextInsertionPointBlinkPeriodOn"];
        const double off_ms = [defaults doubleForKey:@"NSTextInsertionPointBlinkPeriodOff"];
        if (on_ms <= 0.0 && off_ms <= 0.0) {
            return 0.5;
        }
        if (on_ms <= 0.0 || off_ms <= 0.0) {
            return 0.0;  // One phase disabled means "do not blink".
        }
        return (on_ms + off_ms) / 2000.0;
    }();
    return blink_time;
}

void px_show_error(px_window_t* parent, const char* message) {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.alertStyle = NSAlertStyleCritical;
    alert.messageText = message ? @(message) : @"";
    [alert addButtonWithTitle:@"OK"];

    if (parent && parent->window) {
        [alert beginSheetModalForWindow:parent->window
                      completionHandler:^(NSModalResponse){
                      }];
    } else {
        [alert runModal];
    }
}

void px_open_url(const char* url) {
    if (!url) {
        return;
    }
    NSURL* ns_url = [NSURL URLWithString:@(url)];
    if (ns_url) {
        [NSWorkspace.sharedWorkspace openURL:ns_url];
    }
}

double px_now() {
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - g_start;
    return elapsed.count();
}
