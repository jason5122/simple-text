#import "OpenGLLayer.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>

@implementation OpenGLLayer

// Numbers (and file layout) is in the typical order that these messages will be sent.

// 1)   [Optional] This message is sent prior to get a description of the pixel format that you
// need to render your content.
//      This pixel format should use the given display mask for the kCGLPFADisplayMask format
// attribute for optimal performance.
- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    // The default is fine for this demonstration.
    return [super copyCGLPixelFormatForDisplayMask:mask];
}

// 2)   [Optional] This message is sent prior to rendering to create a context to render to.
//      You would typically override this method if you needed to specify a share context to share
// OpenGL resources.         This is also an ideal location to do any initialization that is
// necessary for the context returned
- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    // Default handling is fine for this demonstration.
    return [super copyCGLContextForPixelFormat:pixelFormat];
}

// 3)   [Optional] This message is sent prior to rendering a frame to determine if the layer has
// new content.
//      If this layer is asynchronous and has internal knowledge of when the content has changed
//      then you would want to implement this message in order to inform Core Animation when you
// have new content.         If the layer is not asynchronous, or you always have new content you
// do not need to implement this message.
- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    // Just like the default, we'll just always return YES and always refresh.
    // You normally would not override this method to do this.
    return YES;
}

// 4)   [Required] Implement this message in order to actually draw anything.
//      Typically you will do the following when you recieve this message:
//      1. Draw your OpenGL content! (the current context has already been set)
//      2. call [super drawInContext:pixelFormat:forLayerTime:displayTime:] to finalize the layer
// content, or call glFlush().       NOTE: The viewport has already been set correctly by the time
// this message is sent, so you do not need to set it yourself.       The viewport is automatically
// updated whenever the layer is displayed (that is, when the -display message is sent). This is
// arranged for when you send the -setNeedsDisplay message, or when the needsDisplayOnBoundsChange
// property is set to YES         and the layer's size changes.
- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    // Set the current context to the one given to us.
    CGLSetCurrentContext(glContext);

    // We're just going to draw a single red quad spinning around based on the current time.
    // Nothing particularly fancy.
    GLfloat rotate = timeInterval * 120.0;  // 120 degrees per second!
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotatef(rotate, 0.0, 0.0, 1.0);
    glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0);
    glVertex2f(-0.5, -0.5);
    glVertex2f(-0.5, 0.5);
    glVertex2f(0.5, 0.5);
    glVertex2f(0.5, -0.5);
    glEnd();
    glPopMatrix();

    // Call super to finalize the drawing. By default all it does is call glFlush().
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];
}

// 5)   [Optional] Called when the context given is no longer needed.
//      This is a good place to cleaup any context specific initializations that are necessary
//(which is usually none).      It is not typical to override this method.
- (void)releaseCGLContext:(CGLContextObj)glContext {
    [super releaseCGLContext:glContext];
}

// 6)   [Optional] Called when the given pixel format is no longer needed.
//      It is not typical to override this method.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
    [super releaseCGLPixelFormat:pixelFormat];
}

@end
