// Shared internals of the Cocoa backend. Objective-C++ only; portable code sees px_window_t as an
// opaque forward declaration.

#pragma once

#import <AppKit/AppKit.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>

#include <atomic>
#include <vector>

#include "experiments/platform/px/px.h"

@class PXView;
@class PXWindowDelegate;
@class PXOpenGLLayer;

// ST's px_window_t is ~0x578 bytes with the NSWindow at +0, the view at +8, the handler at +0x10,
// the CVDisplayLink at +0x28, an in-draw flag at +0x38, the dirty vector at +0x510/+0x518 and the
// use_gl flag at +0x570. Same members, laid out for readability rather than by offset.
struct px_window_t {
    NSWindow* window = nil;
    PXView* view = nil;
    PXWindowDelegate* delegate = nil;

    px_window_event_handler* handler = nullptr;

    CVDisplayLinkRef display_link = nullptr;

    // Set the first time the layer's draw callback runs. send_event gates its repaint flush on
    // this (ST reads the same flag at px_window_t+0x38, which drawInCGLContext: sets to 1), so
    // nothing tries to flush before the GL drawable exists.
    bool did_first_paint = false;
    bool closing = false;
    bool tracking_mouse = false;

    // -[PXView makeBackingLayer] branches on this: true installs the CAOpenGLLayer, false falls
    // through to super and the -[PXView drawRect:] software path.
    bool use_gl = true;

    fcolor background{0.0f, 0.0f, 0.0f, 1.0f};
    px_cursor_t cursor = PX_CURSOR_ARROW;

    // Pending regions, in window-space points. Drained by flush_dirty_rects into the layer.
    std::vector<rect> dirty;

    // Keep one main-thread hop outstanding while allowing newer display frames to replace its
    // timestamp.
    std::atomic<bool> tick_pending{false};
    std::atomic<double> latest_animation_time{0.0};

    // Timestamp of the last flush, used to rate-limit event-driven repaints the way ST does.
    double last_flush = 0.0;

    // Native mouse-event timestamp for optional presentation-latency instrumentation. NSEvent's
    // clock and CVTimeStamp.hostTime are both based on system uptime, so they can be compared
    // without involving wall-clock time. The serial is published last and acquired by the render
    // thread.
    std::atomic<double> last_motion_event_time{0.0};
    std::atomic<uint64_t> motion_serial{0};
};

// Implemented in px_window.mm.
void px_mac_send_event(px_window_t* window, px_event_t* event);
void px_mac_flush_dirty_rects(px_window_t* window);
void px_mac_update_display_link(px_window_t* window);
void px_mac_dispatch_post_event_callbacks();
void px_mac_install_before_waiting_observer();
double px_mac_host_time_seconds(uint64_t ticks);

// Converts a Cocoa rect in the view's (flipped) coordinate space to our top-left point space.
rect px_mac_rect_from_ns(NSRect r);
NSRect px_mac_ns_from_rect(rect r);

// Implemented in px_gl_layer.mm.
CALayer* px_mac_make_gl_layer(px_window_t* window);
void px_mac_gl_layer_add_dirty(CALayer* layer, rect r);

// Implemented in px_keycode.mm.
uint32_t px_mac_modifiers_from_ns(NSEventModifierFlags flags);
px_key px_mac_keycode_to_px_key(NSString* chars_ignoring_modifiers,
                                unsigned short key_code,
                                NSEventModifierFlags flags);
