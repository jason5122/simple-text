#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    NSView* mainView;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
