#include "base/apple/ca_display_link_mac.h"
#include "base/check.h"
#include <AppKit/AppKit.h>
#include <QuartzCore/CADisplayLink.h>

API_AVAILABLE(macos(14.0))
@interface CADisplayLinkTarget : NSObject {
    std::function<void()> _callback;
}
- (void)step:(CADisplayLink*)displayLink;
- (void)setCallback:(std::function<void()>)callback;
@end

@implementation CADisplayLinkTarget
- (void)step:(CADisplayLink*)displayLink {
    DCHECK(_callback);
    _callback();
}

- (void)setCallback:(std::function<void()>)callback {
    _callback = std::move(callback);
}
@end

namespace base::apple {

struct CADisplayLinkMac::Impl {
    CADisplayLink* __strong display_link API_AVAILABLE(macos(14.0));
    CADisplayLinkTarget* __strong target API_AVAILABLE(macos(14.0));
};

namespace {
NSScreen* get_ns_screen_from_display_id(CGDirectDisplayID display_id) {
    for (NSScreen* screen in NSScreen.screens) {
        CGDirectDisplayID screen_display_id = kCGNullDirectDisplay;
        screen_display_id = [screen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];

        // TODO: Support macOS 26.
        // if (@available(macOS 26, *)) {
        //     screen_display_id = screen.CGDirectDisplayID;
        // } else {
        //     screen_display_id = [screen.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
        // }

        if (screen_display_id == display_id) {
            return screen;
        }
    }
    return nullptr;
}
}  // namespace

CADisplayLinkMac::CADisplayLinkMac(CGDirectDisplayID display_id)
    : impl_(std::make_unique<Impl>()) {}

CADisplayLinkMac::~CADisplayLinkMac() {
    weak_factory_.InvalidateWeakPtrs();

    // We must manually invalidate the CADisplayLink as its addToRunLoop keeps strong reference to
    // its target. Thus, releasing our impl_ won't really result in destroying the object.
    if (@available(macos 14.0, *)) {
        if (impl_->display_link) {
            [impl_->display_link invalidate];
        }
    }
}

std::unique_ptr<CADisplayLinkMac> CADisplayLinkMac::create_for_display(
    CGDirectDisplayID display_id) {
    if (@available(macos 14.0, *)) {
        std::unique_ptr<CADisplayLinkMac> display_link(new CADisplayLinkMac(display_id));

        NSScreen* screen = get_ns_screen_from_display_id(display_id);
        if (!screen) return nullptr;

        auto* impl = display_link->impl_.get();
        impl->target = [[CADisplayLinkTarget alloc] init];
        impl->display_link = [screen displayLinkWithTarget:impl->target selector:@selector(step:)];
        if (!impl->display_link) return nullptr;

        // TODO: Investigate run loop modes. Apparently `NSRunLoopCommonModes` is for scrolling.
        [impl->display_link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];

        // Set the CADisplayLinkTarget's callback to call back into the C++ code.
        auto weak = display_link->weak_factory_.GetWeakPtr();
        [impl->target setCallback:[weak]() {
            if (!weak) return;
            weak->tick_callback_();
        }];

        return display_link;
    }
    return nullptr;
}

void CADisplayLinkMac::set_callback(TickCallback callback) {
    tick_callback_ = std::move(callback);
}

void CADisplayLinkMac::start() {
    if (@available(macos 14.0, *)) {
        impl_->display_link.paused = NO;
    }
}

void CADisplayLinkMac::stop() {
    if (@available(macos 14.0, *)) {
        impl_->display_link.paused = YES;
    }
}

}  // namespace base::apple
