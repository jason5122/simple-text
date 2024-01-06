#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface Renderer : NSObject

- (nonnull instancetype)initWithMetalDevice:(nonnull id<MTLDevice>)device
                        drawablePixelFormat:(MTLPixelFormat)drawablePixelFormat;

- (void)renderToMetalLayer:(nonnull CAMetalLayer*)metalLayer;

- (void)drawableResize:(CGSize)drawableSize;

@end
