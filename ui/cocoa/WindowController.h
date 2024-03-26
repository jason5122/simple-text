#import <Cocoa/Cocoa.h>

#include "ui/cocoa/EditorView.h"

@interface WindowController : NSWindowController {
    EditorView* editorView;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
