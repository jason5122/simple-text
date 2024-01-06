#import <Foundation/Foundation.h>

@interface BoingRenderer : NSObject

- (void)render;
- (void)makeOrthographicForWidth:(CGFloat)width height:(CGFloat)height;
- (void)updateForWidth:(CGFloat)width height:(CGFloat)height;

@end
