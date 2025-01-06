#pragma once

#include <Cocoa/Cocoa.h>

class AppMenuBridge;

@interface AppMenuCocoaController : NSObject <NSMenuDelegate, NSMenuItemValidation>

- (instancetype)initWithBridge:(AppMenuBridge*)bridge;

@end
