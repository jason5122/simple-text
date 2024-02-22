#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
@interface EditorViewNew : NSOpenGLView {
    NSOpenGLPixelFormat* pixelFormat;
    CVDisplayLinkRef displayLink;
}
@end
#pragma clang diagnostic pop
