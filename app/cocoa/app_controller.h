#pragma once

#include <Cocoa/Cocoa.h>

// This handles things like responding to menus when there are no windows open, etc and acts as the
// NSApplication delegate.
@interface AppController
    : NSObject <NSApplicationDelegate, NSMenuDelegate, NSUserInterfaceValidations>

// The app-wide singleton AppController. Guaranteed to be the delegate of NSApp. Guaranteed to not
// be nil.
@property(readonly, nonatomic, class) AppController* sharedController;

// Use the `sharedController` property.
- (instancetype)init NS_UNAVAILABLE;

@end
