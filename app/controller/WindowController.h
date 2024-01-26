#import "view/EditorView.h"
// #import "view/OpenGLLayer.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    EditorView* editorView;
    OpenGLLayer* openGLLayer;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
