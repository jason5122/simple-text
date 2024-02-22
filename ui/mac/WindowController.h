#import "ui/mac/EditorView.h"
#import "ui/mac/EditorViewNew.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    EditorView* editorView;
    EditorViewNew* editorViewNew;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
