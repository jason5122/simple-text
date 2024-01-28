#import "ui/mac/EditorView.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    EditorView* editorView;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end