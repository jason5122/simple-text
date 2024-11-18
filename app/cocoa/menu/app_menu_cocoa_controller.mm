#include "app_menu_cocoa_controller.h"

@implementation AppMenuCocoaController {
    AppMenuBridge* bridge;
}

- (instancetype)initWithBridge:(AppMenuBridge*)theBridge {
    if (self = [super init]) {
        bridge = theBridge;
    }
    return self;
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem {
    return YES;
}

@end
