#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

@interface EditorViewNew : NSOpenGLView {
    NSOpenGLPixelFormat* pixelFormat;
    CVDisplayLinkRef displayLink;
}
@end
