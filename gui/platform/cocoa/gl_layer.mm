#include "gui/platform/cocoa/gl_layer.h"
#include <Cocoa/Cocoa.h>
#include <fmt/base.h>

@interface GLLayer () {
    gui::WindowWidget* app_window;
    gui::DisplayGL* display_gl;
    int old_width;
    int old_height;
    CVDisplayLinkRef display_link;

    int64_t first_frame_time;
}

- (void)frameCallback:(const CVTimeStamp*)outputTime;

@end

namespace {
CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                             const CVTimeStamp* now,
                             const CVTimeStamp* outputTime,
                             CVOptionFlags flagsIn,
                             CVOptionFlags* flagsOut,
                             void* data) {
    GLLayer* gl_layer = static_cast<GLLayer*>(data);
    [gl_layer frameCallback:outputTime];
    return kCVReturnSuccess;
}
}  // namespace

@implementation GLLayer

- (void)frameCallback:(const CVTimeStamp*)outputTime {
    int scale = outputTime->videoTimeScale;
    // Convert scale from seconds to milliseconds.
    scale /= 1000;

    int64_t frame_time = outputTime->videoTime / scale;
    if (first_frame_time == 0) {
        first_frame_time = frame_time;
    }

    int64_t ms = frame_time - first_frame_time;

    // CVDisplayLink can only draw on the main thread.
    dispatch_async(dispatch_get_main_queue(), ^{
      if (app_window) {
          app_window->on_frame(ms);
      }
    });
}

- (instancetype)initWithAppWindow:(gui::WindowWidget*)appWindow
                        displayGL:(gui::DisplayGL*)displayGL {
    self = [super init];
    if (self) {
        app_window = appWindow;
        display_gl = displayGL;
        old_width = -1;
        old_height = -1;
        first_frame_time = 0;

        // Create a display link capable of being used with all active displays.
        auto context = display_gl->context();
        auto pixel_format = display_gl->pixelFormat();
        CVDisplayLinkCreateWithActiveCGDisplays(&display_link);
        CVDisplayLinkSetOutputCallback(display_link, &DisplayLinkCallback, self);
        CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(display_link, context, pixel_format);
    }
    return self;
}

- (void)invalidateAppWindowPointer {
    app_window = nullptr;
    CVDisplayLinkStop(display_link);
    CVDisplayLinkRelease(display_link);
}

- (void)setAutoRedraw:(bool)autoRedraw {
    if (autoRedraw) {
        CVDisplayLinkStart(display_link);
    } else {
        CVDisplayLinkStop(display_link);
    }
}

- (int)framesPerSecond {
    CVTime cv_time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(display_link);
    if (cv_time.flags & kCVTimeIsIndefinite) {
        fmt::println("Error: Could not get CVDisplayLink refresh rate.");
        std::abort();
    }
    int64_t fps = cv_time.timeScale / cv_time.timeValue;
    return fps;
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
    return display_gl->pixelFormat();
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat {
    // Call OpenGL activation callback.
    CGLSetCurrentContext(display_gl->context());
    if (app_window) {
        app_window->on_opengl_activate();
    }
    return display_gl->context();
}

- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return true;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];

    CGLSetCurrentContext(glContext);

    if (old_width != self.frame.size.width || old_height != self.frame.size.height) {
        old_width = self.frame.size.width;
        old_height = self.frame.size.height;
        if (app_window) {
            int scale = self.contentsScale;
            app_window->set_width(old_width * scale);
            app_window->set_height(old_height * scale);
            app_window->layout();
        }
    }

    if (app_window) {
        app_window->draw();
    }

    // Calls glFlush() by default.
    [super drawInCGLContext:glContext
                pixelFormat:pixelFormat
               forLayerTime:timeInterval
                displayTime:timeStamp];

    // TODO: For debugging; remove this.
    // [NSApp terminate:nil];
}

// We shouldn't release the CGLContextObj since it isn't owned by this object.
- (void)releaseCGLContext:(CGLContextObj)glContext {
}

// We shouldn't release the CGLPixelFormatObj since it isn't owned by this object.
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pixelFormat {
}

@end
