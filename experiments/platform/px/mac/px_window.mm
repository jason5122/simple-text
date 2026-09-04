// PXWindow / PXWindowDelegate / PXView, and the window half of the flat px API.
//
// Structure mirrored from ST's binary:
//
//   * PXWindow : NSWindow, PXView : NSView (not NSOpenGLView), PXWindowDelegate is a separate
//     object conforming to NSWindowDelegate. PXView conforms to NSTextInputClient and forwards to
//     whatever get_input_client() returns.
//
//   * Every AppKit entry point does the same three things: memset a px_event_t, fill the fields
//     that matter for its tag, and call one funnel, send_event(px_window_t*, px_event_t*). The
//     funnel is the only place that touches the handler's vtable.
//     -[PXWindowDelegate windowDidResize:] is the clearest example: `mov w8, #0x8` for the tag,
//     then `bl send_event`.
//
//   * send_event's post-condition, decoded at 0x1002c3008: after dispatching, if the window has
//     painted at least once (+0x38 == 1) and the event tag is >= 2 -- i.e. not a key or character
//     event -- and enough wall time has passed, it calls pre_paint() and flush_dirty_rects().
//     Then dispatch_post_event_callbacks() runs unconditionally.

#include "experiments/platform/px/mac/px_mac_private.h"

#include "experiments/platform/px/skia_render_context.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

// Called unconditionally by Sublime immediately after NSWindow initialization. AppKit still
// implements this private selector despite no longer declaring it in the public SDK.
@interface NSWindow (PXOptimizedDrawing)
- (void)useOptimizedDrawing:(BOOL)flag;
@end

namespace {

// Exact f64 loaded by ST's send_event tail. This is only the eager event-path flush; the
// kCFRunLoopBeforeWaiting observer below is the reliable end-of-turn submission point.
constexpr double kMinEventFlushInterval = 1.0 / 60.0;

std::vector<std::function<void()>>& post_event_callbacks() {
    static std::vector<std::function<void()>> callbacks;
    return callbacks;
}

// Scratch storage for the char* array handed to drag-drop and drop-file events. Valid only for the
// duration of the dispatch, which is exactly the contract px_event_t::paths documents.
struct PathList {
    std::vector<std::string> storage;
    std::vector<const char*> pointers;

    // Accepts an array of NSURL or NSString; the pasteboard hands back the former and
    // application:openFiles: the latter.
    void assign(NSArray* urls_or_paths) {
        storage.clear();
        pointers.clear();
        for (id item in urls_or_paths) {
            NSString* path = nil;
            if ([item isKindOfClass:[NSURL class]]) {
                path = static_cast<NSURL*>(item).path;
            } else if ([item isKindOfClass:[NSString class]]) {
                path = static_cast<NSString*>(item);
            }
            if (path) {
                storage.emplace_back(path.UTF8String);
            }
        }
        pointers.reserve(storage.size());
        for (const std::string& s : storage) {
            pointers.push_back(s.c_str());
        }
    }

    const char* const* data() const { return pointers.empty() ? nullptr : pointers.data(); }
    int count() const { return static_cast<int>(pointers.size()); }
};

PathList& drag_paths() {
    static PathList paths;
    return paths;
}

void copy_paths_from_pasteboard(NSPasteboard* pb, PathList* out) {
    NSArray<NSURL*>* urls = [pb readObjectsForClasses:@[ [NSURL class] ] options:nil];
    out->assign(urls);
}

NSCursor* ns_cursor(px_cursor_t cursor) {
    switch (cursor) {
    case PX_CURSOR_IBEAM:
        return [NSCursor IBeamCursor];
    case PX_CURSOR_CROSSHAIR:
        return [NSCursor crosshairCursor];
    case PX_CURSOR_POINTING_HAND:
        return [NSCursor pointingHandCursor];
    case PX_CURSOR_RESIZE_LEFT_RIGHT:
        return [NSCursor resizeLeftRightCursor];
    case PX_CURSOR_RESIZE_UP_DOWN:
        return [NSCursor resizeUpDownCursor];
    case PX_CURSOR_ARROW:
    default:
        return [NSCursor arrowCursor];
    }
}

px_range_t px_range_from_ns(NSRange r) {
    if (r.location == NSNotFound) {
        return px_range_t::none();
    }
    return px_range_t{static_cast<int64_t>(r.location), static_cast<int64_t>(r.length)};
}

NSRange ns_range_from_px(px_range_t r) {
    if (!r.valid()) {
        return NSMakeRange(NSNotFound, 0);
    }
    return NSMakeRange(static_cast<NSUInteger>(r.location), static_cast<NSUInteger>(r.length));
}

std::vector<px_window_t*>& all_windows() {
    static std::vector<px_window_t*> windows;
    return windows;
}

dummy_px_window_event_handler& dummy_handler() {
    static dummy_px_window_event_handler handler;
    return handler;
}

}  // namespace

rect px_mac_rect_from_ns(NSRect r) {
    return rect{r.origin.x, r.origin.y, r.size.width, r.size.height};
}

NSRect px_mac_ns_from_rect(rect r) { return NSMakeRect(r.x, r.y, r.w, r.h); }

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// PXView
// ─────────────────────────────────────────────────────────────────────────────────────────────────

@interface PXView : NSView <NSTextInputClient> {
    px_window_t* _pxw;
    NSTrackingArea* _trackingArea;
}
- (instancetype)initWithPXW:(px_window_t*)pxw;
@end

@implementation PXView

- (instancetype)initWithPXW:(px_window_t*)pxw {
    self = [super initWithFrame:NSMakeRect(0, 0, 1, 1)];
    if (self) {
        _pxw = pxw;
        self.wantsLayer = YES;
        self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
    }
    return self;
}

// Top-left origin, so view coordinates match the px coordinate space with no conversion. ST's
// PXView does the same.
- (BOOL)isFlipped {
    return YES;
}

- (BOOL)isOpaque {
    return _pxw && _pxw->background.a >= 1.0f;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (CALayer*)makeBackingLayer {
    // The branch -[PXView makeBackingLayer] makes on pxw->use_gl (+0x570).
    if (_pxw && _pxw->use_gl) {
        return px_mac_make_gl_layer(_pxw);
    }
    return [super makeBackingLayer];
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!_pxw || !_pxw->handler) {
        return;
    }

    _pxw->handler->pre_paint();
    px_mac_dispatch_post_event_callbacks();

    const NSRect* native_dirty = nullptr;
    NSInteger native_dirty_count = 0;
    [self getRectsBeingDrawn:&native_dirty count:&native_dirty_count];

    std::vector<rect> dirty;
    dirty.reserve(static_cast<size_t>(std::max<NSInteger>(native_dirty_count, 1)));
    rect paint_bounds;
    const auto add_dirty = [&](NSRect native) {
        const rect area = px_mac_rect_from_ns(NSIntersectionRect(native, self.bounds));
        if (area.empty()) {
            return;
        }
        dirty.push_back(area);
        if (paint_bounds.empty()) {
            paint_bounds = area;
        } else {
            const double right = std::max(paint_bounds.right(), area.right());
            const double bottom = std::max(paint_bounds.bottom(), area.bottom());
            paint_bounds.x = std::min(paint_bounds.x, area.x);
            paint_bounds.y = std::min(paint_bounds.y, area.y);
            paint_bounds.w = right - paint_bounds.x;
            paint_bounds.h = bottom - paint_bounds.y;
        }
    };
    for (NSInteger i = 0; i < native_dirty_count; ++i) {
        add_dirty(native_dirty[i]);
    }
    if (dirty.empty()) {
        add_dirty(dirtyRect);
    }
    if (dirty.empty()) {
        return;
    }

    const double dpi_scale = px_window_dpi_scale_factor(_pxw);
    self.layer.contentsScale = dpi_scale;
    const vec2 logical_size = px_window_size(_pxw);
    const int width = static_cast<int>(logical_size.x * dpi_scale);
    const int height = static_cast<int>(logical_size.y * dpi_scale);
    if (width <= 0 || height <= 0 ||
        static_cast<size_t>(width) > std::numeric_limits<size_t>::max() /
                                         (static_cast<size_t>(height) * sizeof(uint32_t))) {
        return;
    }

    // Sublime uses one process-wide scratch allocation for software paints. It only needs to
    // survive until CGContextDrawImage returns: AppKit's backing store retains all pixels outside
    // the current damage region.
    static std::vector<uint32_t> pixels;
    const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    pixels.resize(pixel_count);
    if (![self isOpaque]) {
        std::fill(pixels.begin(), pixels.end(), 0u);
    }

    recti pixel_clip{
        std::max(0, static_cast<int>(std::floor(paint_bounds.x * dpi_scale))),
        std::max(0, static_cast<int>(std::floor(paint_bounds.y * dpi_scale))),
        std::min(width, static_cast<int>(std::ceil(paint_bounds.right() * dpi_scale))),
        std::min(height, static_cast<int>(std::ceil(paint_bounds.bottom() * dpi_scale))),
    };
    const size_t row_bytes = static_cast<size_t>(width) * sizeof(uint32_t);
    skia_render_context rc(px_pixel_buffer{pixels.data(), width, height, row_bytes}, pixel_clip,
                           dpi_scale);
    if (!rc.valid()) {
        return;
    }
    _pxw->handler->paint(&rc, paint_bounds, dirty.data(), static_cast<int>(dirty.size()));

    CGContextRef destination = NSGraphicsContext.currentContext.CGContext;
    if (!destination) {
        return;
    }
    CGColorSpaceRef color_space = _pxw->window.colorSpace.CGColorSpace;
    if (color_space) {
        CGColorSpaceRetain(color_space);
    } else {
        color_space = CGColorSpaceCreateDeviceRGB();
    }
    if (!color_space) {
        return;
    }
    CGDataProviderRef provider = CGDataProviderCreateWithData(
        nullptr, pixels.data(), row_bytes * static_cast<size_t>(height), nullptr);
    const CGBitmapInfo bitmap_info =
        static_cast<CGBitmapInfo>(static_cast<uint32_t>(kCGImageAlphaPremultipliedFirst) |
                                  static_cast<uint32_t>(kCGBitmapByteOrder32Little));
    CGImageRef image = provider
                           ? CGImageCreate(static_cast<size_t>(width), static_cast<size_t>(height),
                                           8, 32, row_bytes, color_space, bitmap_info, provider,
                                           nullptr, false, kCGRenderingIntentDefault)
                           : nullptr;
    if (image) {
        CGContextSaveGState(destination);
        CGContextClipToRect(destination, CGRectMake(paint_bounds.x, paint_bounds.y, paint_bounds.w,
                                                    paint_bounds.h));
        CGContextTranslateCTM(destination, 0.0, logical_size.y);
        CGContextScaleCTM(destination, 1.0, -1.0);
        CGContextSetBlendMode(destination, kCGBlendModeCopy);
        CGContextDrawImage(destination, CGRectMake(0.0, 0.0, logical_size.x, logical_size.y),
                           image);
        CGContextRestoreGState(destination);
        CGImageRelease(image);
    }
    if (provider) {
        CGDataProviderRelease(provider);
    }
    CGColorSpaceRelease(color_space);

    _pxw->did_first_paint = true;
    _pxw->last_flush = px_now();
}

// ── tracking ────────────────────────────────────────────────────────────────────────────────────

- (void)updateTrackingAreas {
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
    }
    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                                          NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
    _trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                 options:options
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:_trackingArea];
    [super updateTrackingAreas];
}

- (void)resetCursorRects {
    if (!_pxw || !_pxw->handler) {
        return;
    }
    // ST asks the handler per-point via calculate_cursor. Cocoa wants rects, so the whole view
    // gets the cursor for the current mouse location and the handler is re-asked on every motion.
    const NSPoint p = [self convertPoint:self.window.mouseLocationOutsideOfEventStream
                                fromView:nil];
    const px_cursor_t cursor = _pxw->handler->calculate_cursor(vec2{p.x, p.y});
    [self addCursorRect:self.bounds cursor:ns_cursor(cursor)];
}

// ── mouse ───────────────────────────────────────────────────────────────────────────────────────

- (vec2)pxPointFor:(NSEvent*)event {
    const NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    return vec2{p.x, p.y};
}

- (void)sendMouseButton:(NSEvent*)event button:(px_mouse_button)button pressed:(BOOL)pressed {
    px_event_t e{};
    e.type = PX_EVENT_MOUSE_BUTTON;
    e.pos = [self pxPointFor:event];
    e.button = button;
    e.pressed = pressed == YES;
    e.click_count = static_cast<int>(event.clickCount);
    e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
    px_mac_send_event(_pxw, &e);
}

- (void)sendMouseMotion:(NSEvent*)event {
    px_event_t e{};
    e.type = PX_EVENT_MOUSE_MOTION;
    e.pos = [self pxPointFor:event];
    e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
    _pxw->last_motion_event_time.store(event.timestamp, std::memory_order_relaxed);
    _pxw->motion_serial.fetch_add(1, std::memory_order_release);
    px_mac_send_event(_pxw, &e);
}

- (void)mouseDown:(NSEvent*)event {
    [self sendMouseButton:event button:PX_MOUSE_LEFT pressed:YES];
}
- (void)mouseUp:(NSEvent*)event {
    [self sendMouseButton:event button:PX_MOUSE_LEFT pressed:NO];
}
- (void)rightMouseDown:(NSEvent*)event {
    [self sendMouseButton:event button:PX_MOUSE_RIGHT pressed:YES];
}
- (void)rightMouseUp:(NSEvent*)event {
    [self sendMouseButton:event button:PX_MOUSE_RIGHT pressed:NO];
}
- (void)otherMouseDown:(NSEvent*)event {
    [self sendMouseButton:event button:PX_MOUSE_MIDDLE pressed:YES];
}
- (void)otherMouseUp:(NSEvent*)event {
    [self sendMouseButton:event button:PX_MOUSE_MIDDLE pressed:NO];
}
- (void)mouseMoved:(NSEvent*)event {
    [self sendMouseMotion:event];
}
- (void)mouseDragged:(NSEvent*)event {
    [self sendMouseMotion:event];
}
- (void)rightMouseDragged:(NSEvent*)event {
    [self sendMouseMotion:event];
}
- (void)otherMouseDragged:(NSEvent*)event {
    [self sendMouseMotion:event];
}

- (void)mouseExited:(NSEvent*)event {
    px_event_t e{};
    e.type = PX_EVENT_MOUSE_LEAVE;
    e.pos = [self pxPointFor:event];
    e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
    px_mac_send_event(_pxw, &e);
}

- (void)scrollWheel:(NSEvent*)event {
    px_event_t e{};
    e.type = PX_EVENT_SCROLL;
    e.pos = [self pxPointFor:event];
    e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
    e.precise_scroll = event.hasPreciseScrollingDeltas == YES;
    if (e.precise_scroll) {
        e.scroll_delta = vec2{event.scrollingDeltaX, event.scrollingDeltaY};
    } else {
        // Line-based wheels report in lines; normalise to something pixel-ish so consumers see one
        // unit system.
        constexpr double kLineHeight = 16.0;
        e.scroll_delta =
            vec2{event.scrollingDeltaX * kLineHeight, event.scrollingDeltaY * kLineHeight};
    }
    px_mac_send_event(_pxw, &e);
}

// ── keyboard ────────────────────────────────────────────────────────────────────────────────────

- (void)keyDown:(NSEvent*)event {
    [NSCursor setHiddenUntilMouseMoves:YES];

    const BOOL hadMarkedText = [self hasMarkedText];

    // Bindings get first refusal, then the input context. That ordering is what ST does: it sends
    // the key event to the handler, and only hands the event to the NSTextInputContext if the app
    // did not consume it. An editor needs it this way, or Cmd-S would type an 's'.
    BOOL consumed = NO;
    if (!hadMarkedText && _pxw && _pxw->handler) {
        px_event_t e{};
        e.type = PX_EVENT_KEY;
        e.key = px_mac_keycode_to_px_key(event.charactersIgnoringModifiers, event.keyCode,
                                         event.modifierFlags);
        e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
        e.pressed = true;
        e.repeat = event.isARepeat == YES;
        e.window = _pxw;
        consumed = _pxw->handler->handle_event(&e) ? YES : NO;
        px_mac_dispatch_post_event_callbacks();
    }

    if (consumed) {
        [self.inputContext discardMarkedText];
        return;
    }
    [self.inputContext handleEvent:event];
}

- (void)keyUp:(NSEvent*)event {
    px_event_t e{};
    e.type = PX_EVENT_KEY;
    e.key = px_mac_keycode_to_px_key(event.charactersIgnoringModifiers, event.keyCode,
                                     event.modifierFlags);
    e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
    e.pressed = false;
    px_mac_send_event(_pxw, &e);
}

- (void)flagsChanged:(NSEvent*)event {
    // A modifier-only transition. Reported as a key event with no key so the app can refresh any
    // modifier-dependent state (hover decorations, alt-drag modes).
    px_event_t e{};
    e.type = PX_EVENT_KEY;
    e.key = PX_KEY_NONE;
    e.modifiers = px_mac_modifiers_from_ns(event.modifierFlags);
    px_mac_send_event(_pxw, &e);
}

// ── drag and drop ───────────────────────────────────────────────────────────────────────────────

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    if (!_pxw || !_pxw->handler) {
        return NSDragOperationNone;
    }
    copy_paths_from_pasteboard(sender.draggingPasteboard, &drag_paths());
    const NSPoint p = [self convertPoint:sender.draggingLocation fromView:nil];
    const bool accept =
        _pxw->handler->drag_drop_enter(vec2{p.x, p.y}, drag_paths().data(), drag_paths().count());
    return accept ? NSDragOperationCopy : NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    if (!_pxw || !_pxw->handler) {
        return NSDragOperationNone;
    }
    const NSPoint p = [self convertPoint:sender.draggingLocation fromView:nil];
    const bool accept =
        _pxw->handler->drag_drop_motion(vec2{p.x, p.y}, drag_paths().data(), drag_paths().count());
    return accept ? NSDragOperationCopy : NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    if (_pxw && _pxw->handler) {
        _pxw->handler->drag_drop_exit();
    }
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    if (!_pxw || !_pxw->handler) {
        return NO;
    }
    copy_paths_from_pasteboard(sender.draggingPasteboard, &drag_paths());
    const NSPoint p = [self convertPoint:sender.draggingLocation fromView:nil];
    const bool accepted =
        _pxw->handler->drag_drop_accept(vec2{p.x, p.y}, drag_paths().data(), drag_paths().count());

    if (accepted) {
        // Also surfaced as an event, the way WM_DROPFILES arrives on Windows (tag 11).
        px_event_t e{};
        e.type = PX_EVENT_DROP_FILES;
        e.pos = vec2{p.x, p.y};
        e.paths = drag_paths().data();
        e.path_count = drag_paths().count();
        px_mac_send_event(_pxw, &e);
    }
    return accepted ? YES : NO;
}

// ── NSTextInputClient, forwarded to px_input_client
// ──────────────────────────────────────────────

- (px_input_client*)inputClient {
    return (_pxw && _pxw->handler) ? _pxw->handler->get_input_client() : nullptr;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    NSString* text = [string isKindOfClass:[NSAttributedString class]]
                         ? static_cast<NSAttributedString*>(string).string
                         : static_cast<NSString*>(string);
    if (px_input_client* client = [self inputClient]) {
        client->insert_text(text.UTF8String, px_range_from_ns(replacementRange));
        return;
    }

    // No IME client installed: still report the text, as a character event (tag 1, WM_CHAR's tag).
    px_event_t e{};
    e.type = PX_EVENT_CHARACTER;
    const char* utf8 = text.UTF8String;
    px_set_event_text(&e, utf8 ? utf8 : "", utf8 ? std::strlen(utf8) : 0);
    px_mac_send_event(_pxw, &e);
}

- (void)doCommandBySelector:(SEL)selector {
    if (px_input_client* client = [self inputClient]) {
        client->do_command(sel_getName(selector));
    }
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
    NSString* text = [string isKindOfClass:[NSAttributedString class]]
                         ? static_cast<NSAttributedString*>(string).string
                         : static_cast<NSString*>(string);
    if (px_input_client* client = [self inputClient]) {
        client->set_marked_text(text.UTF8String, px_range_from_ns(selectedRange),
                                px_range_from_ns(replacementRange));
    }
}

- (void)unmarkText {
    if (px_input_client* client = [self inputClient]) {
        client->unmark_text();
    }
}

- (BOOL)hasMarkedText {
    px_input_client* client = [self inputClient];
    return (client && client->has_marked_text()) ? YES : NO;
}

- (NSRange)markedRange {
    px_input_client* client = [self inputClient];
    return client ? ns_range_from_px(client->marked_range()) : NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange {
    px_input_client* client = [self inputClient];
    return client ? ns_range_from_px(client->selected_range()) : NSMakeRange(NSNotFound, 0);
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    if (actualRange) {
        *actualRange = NSMakeRange(NSNotFound, 0);
    }
    return nil;
}

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText {
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    px_input_client* client = [self inputClient];
    if (!client) {
        if (actualRange) {
            *actualRange = NSMakeRange(NSNotFound, 0);
        }
        return NSZeroRect;
    }
    px_range_t actual = px_range_t::none();
    const rect r = client->first_rect_for_range(px_range_from_ns(range), &actual);
    if (actualRange) {
        *actualRange = ns_range_from_px(actual);
    }
    // px space (points, top-left) -> view -> window -> screen.
    const NSRect in_view = px_mac_ns_from_rect(r);
    const NSRect in_window = [self convertRect:in_view toView:nil];
    return [self.window convertRectToScreen:in_window];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    px_input_client* client = [self inputClient];
    if (!client) {
        return NSNotFound;
    }
    const NSRect on_screen = NSMakeRect(point.x, point.y, 0, 0);
    const NSRect in_window = [self.window convertRectFromScreen:on_screen];
    const NSPoint in_view = [self convertPoint:in_window.origin fromView:nil];
    const int64_t index = client->character_index_for_point(vec2{in_view.x, in_view.y});
    return index < 0 ? NSNotFound : static_cast<NSUInteger>(index);
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// PXWindow / PXWindowDelegate
// ─────────────────────────────────────────────────────────────────────────────────────────────────

@interface PXWindow : NSWindow
@end

@implementation PXWindow

// ST overrides only a handful of NSWindow methods; keeping the subclass lets a borderless or
// custom-title-bar window still take key focus.
- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

@end

@interface PXWindowDelegate : NSObject <NSWindowDelegate> {
    px_window_t* _pxw;
}
- (instancetype)initWithPXW:(px_window_t*)pxw;
@end

@implementation PXWindowDelegate

- (instancetype)initWithPXW:(px_window_t*)pxw {
    self = [super init];
    if (self) {
        _pxw = pxw;
    }
    return self;
}

- (void)windowDidResize:(NSNotification*)notification {
    if (!_pxw) {
        return;
    }
    const vec2 size = px_window_size(_pxw);
    px_mark_rect_dirty(_pxw, rect{0.0, 0.0, size.x, size.y});

    px_event_t e{};
    e.type = PX_EVENT_RESIZE;  // tag 8, matching WM_SIZE
    e.size = size;
    e.dpi_scale_factor = px_window_dpi_scale_factor(_pxw);
    px_mac_send_event(_pxw, &e);
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    if (!_pxw) {
        return;
    }
    const double scale = px_window_dpi_scale_factor(_pxw);
    if (_pxw->view.layer) {
        _pxw->view.layer.contentsScale = scale;
    }

    px_event_t e{};
    e.type = PX_EVENT_DPI_CHANGED;  // tag 10, matching WM_DPICHANGED
    e.size = px_window_size(_pxw);
    e.dpi_scale_factor = scale;
    px_mac_send_event(_pxw, &e);
    px_mark_dirty(_pxw);
}

- (void)windowDidChangeScreen:(NSNotification*)notification {
    px_mac_update_display_link(_pxw);
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    px_event_t e{};
    e.type = PX_EVENT_FOCUS_GAINED;  // tag 13, matching WM_SETFOCUS
    px_mac_send_event(_pxw, &e);
}

- (void)windowDidResignKey:(NSNotification*)notification {
    px_event_t e{};
    e.type = PX_EVENT_FOCUS_LOST;  // tag 14, matching WM_KILLFOCUS
    px_mac_send_event(_pxw, &e);
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (!_pxw || !_pxw->handler) {
        return YES;
    }
    // The fast path first, exactly as ST splits it: only fall back to the async prompt when the
    // window admits it has something to lose.
    if (_pxw->handler->can_close_without_prompt()) {
        return YES;
    }
    px_window_t* pxw = _pxw;
    _pxw->handler->try_close([pxw](bool should_close) {
        if (should_close) {
            px_close_window(pxw);
        }
    });
    return NO;
}

- (void)windowWillClose:(NSNotification*)notification {
    if (!_pxw) {
        return;
    }
    px_event_t e{};
    e.type = PX_EVENT_DESTROY;  // tag 9, matching WM_DESTROY
    px_mac_send_event(_pxw, &e);
    _pxw->closing = true;
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// THE FUNNEL
// ─────────────────────────────────────────────────────────────────────────────────────────────────

void px_mac_send_event(px_window_t* window, px_event_t* event) {
    if (!window || !window->handler) {
        return;
    }
    event->window = window;
    window->handler->handle_event(event);

    // send_event's tail. Key (0) and character (1) events skip the repaint flush; everything from
    // mouse button (2) upward can trigger one, rate-limited.
    if (window->did_first_paint && event->type >= PX_EVENT_MOUSE_BUTTON) {
        const double now = px_now();
        if (now - window->last_flush > kMinEventFlushInterval) {
            window->handler->pre_paint();
            px_mac_flush_dirty_rects(window);
        }
    }

    px_mac_dispatch_post_event_callbacks();
}

void px_mac_flush_dirty_rects(px_window_t* window) {
    if (!window || window->dirty.empty()) {
        return;
    }

    for (const rect& r : window->dirty) {
        if (window->use_gl) {
            CALayer* layer = window->view.layer;
            // CAOpenGLLayer does not clip to the invalidated sub-rect, so this only marks the
            // layer as needing display; the authoritative region list is the one handed to paint()
            // below.
            px_mac_gl_layer_add_dirty(layer, r);
            [layer setNeedsDisplayInRect:px_mac_ns_from_rect(r)];
        } else {
            [window->view setNeedsDisplayInRect:px_mac_ns_from_rect(r)];
        }
    }
    window->dirty.clear();
}

void px_mac_dispatch_post_event_callbacks() {
    if (post_event_callbacks().empty()) {
        return;
    }
    // Swap out first: a callback is allowed to enqueue another one, and that one belongs to the
    // next drain, not this loop.
    std::vector<std::function<void()>> pending;
    pending.swap(post_event_callbacks());
    for (const std::function<void()>& fn : pending) {
        fn();
    }
}

namespace {

void update_cursor_from_tracking_rects(px_window_t* window) {
    if (!window || !window->window.isKeyWindow || !window->view || !window->handler) {
        return;
    }

    const NSPoint p = [window->view convertPoint:window->window.mouseLocationOutsideOfEventStream
                                        fromView:nil];
    if (!NSPointInRect(p, window->view.bounds)) {
        return;
    }

    const px_cursor_t cursor = window->handler->calculate_cursor(vec2{p.x, p.y});
    if (cursor != window->cursor) {
        window->cursor = cursor;
        [ns_cursor(cursor) set];
    }
}

void before_waiting_callback(CFRunLoopObserverRef observer,
                             CFRunLoopActivity activity,
                             void* context) {
    // Work from a snapshot so list growth cannot invalidate the iterator. Deferred callbacks are
    // drained only after this loop. ST similarly obtains an NSApp.windows snapshot here.
    const std::vector<px_window_t*> windows = all_windows();
    for (px_window_t* window : windows) {
        if (!window || window->closing || !window->handler) {
            continue;
        }
        window->handler->pre_paint();
        px_mac_flush_dirty_rects(window);
        update_cursor_from_tracking_rects(window);
    }
    px_mac_dispatch_post_event_callbacks();
}

}  // namespace

void px_mac_install_before_waiting_observer() {
    static bool installed = false;
    if (installed) {
        return;
    }

    CFRunLoopObserverContext context{};
    CFRunLoopObserverRef observer = CFRunLoopObserverCreate(
        kCFAllocatorDefault, kCFRunLoopBeforeWaiting, true, 0, &before_waiting_callback, &context);
    if (!observer) {
        return;
    }
    CFRunLoopAddObserver(CFRunLoopGetCurrent(), observer, kCFRunLoopCommonModes);
    CFRelease(observer);  // The run loop owns its reference for the process lifetime.
    installed = true;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {

CVReturn display_link_callback(CVDisplayLinkRef link,
                               const CVTimeStamp* now,
                               const CVTimeStamp* output_time,
                               CVOptionFlags flags_in,
                               CVOptionFlags* flags_out,
                               void* context) {
    px_window_t* window = static_cast<px_window_t*>(context);
    if (!window) {
        return kCVReturnSuccess;
    }
    window->latest_animation_time.store(px_mac_host_time_seconds(output_time->hostTime),
                                        std::memory_order_release);
    if (window->tick_pending.exchange(true, std::memory_order_acq_rel)) {
        return kCVReturnSuccess;
    }
    dispatch_async(dispatch_get_main_queue(), ^{
      // Clear first so a display frame arriving during this callback queues the next drain.
      window->tick_pending.store(false, std::memory_order_release);
      if (window->closing || !window->handler) {
          return;
      }
      const double target_time = window->latest_animation_time.load(std::memory_order_acquire);
      window->handler->animation_tick(target_time);
    });
    return kCVReturnSuccess;
}

}  // namespace

void px_mac_update_display_link(px_window_t* window) {
    if (!window || !window->display_link || !window->window.screen) {
        return;
    }
    NSNumber* screen_number = window->window.screen.deviceDescription[@"NSScreenNumber"];
    if (screen_number) {
        CVDisplayLinkSetCurrentCGDisplay(window->display_link, screen_number.unsignedIntValue);
    }
}

// ─────────────────────────────────────────────────────────────────────────────────────────────────
// FLAT API: WINDOWS
// ─────────────────────────────────────────────────────────────────────────────────────────────────

px_window_t* px_create_window(px_window_event_handler* handler,
                              px_window_t* parent,
                              double width,
                              double height,
                              const char* title,
                              fcolor background,
                              uint32_t flags) {
    px_window_t* pxw = new px_window_t();
    pxw->handler = handler ? handler : &dummy_handler();
    pxw->background = background;
    pxw->use_gl = getenv("PX_NO_GL") == nullptr && !(flags & PX_WINDOW_SOFTWARE);

    NSWindowStyleMask mask = 0;
    if (flags & PX_WINDOW_TITLED) mask |= NSWindowStyleMaskTitled;
    if (flags & PX_WINDOW_CLOSABLE) mask |= NSWindowStyleMaskClosable;
    if (flags & PX_WINDOW_RESIZABLE) mask |= NSWindowStyleMaskResizable;
    if (flags & PX_WINDOW_MINIATURIZABLE) mask |= NSWindowStyleMaskMiniaturizable;

    pxw->window = [[PXWindow alloc] initWithContentRect:NSMakeRect(0, 0, width, height)
                                              styleMask:mask
                                                backing:NSBackingStoreBuffered
                                                  defer:NO
                                                 screen:NSScreen.mainScreen];
    [pxw->window useOptimizedDrawing:YES];
    if (flags & PX_WINDOW_POPUP_LEVEL) {
        pxw->window.level = static_cast<NSWindowLevel>(101);
    }
    if (flags & PX_WINDOW_TRANSPARENT) {
        pxw->window.opaque = NO;
        pxw->window.hasShadow = NO;
        pxw->window.backgroundColor = NSColor.clearColor;
    }
    pxw->window.releasedWhenClosed = NO;
    pxw->window.title = title ? @(title) : @"";
    // This is used by Sublime for the Adaptive Theme.
    // pxw->window.backgroundColor = [NSColor colorWithSRGBRed:background.r
    //                                                  green:background.g
    //                                                   blue:background.b
    //                                                  alpha:background.a];

    pxw->view = [[PXView alloc] initWithPXW:pxw];
    pxw->window.contentView = pxw->view;
    [pxw->view registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];

    pxw->delegate = [[PXWindowDelegate alloc] initWithPXW:pxw];
    pxw->window.delegate = pxw->delegate;

    [pxw->window center];

    // One CVDisplayLink per window, retargeted whenever the window changes screen. The explicit
    // opt-out makes the event-driven experiment genuine. PX_NO_ANIMATION is accepted as a
    // compatibility alias for the demo's existing A/B command line. PX_KEEP_DISPLAY_LINK lets the
    // latency probe isolate display-link participation while leaving the demo animation disabled.
    const bool keep_display_link = getenv("PX_KEEP_DISPLAY_LINK") != nullptr;
    const bool disable_display_link = getenv("PX_NO_DISPLAY_LINK") != nullptr ||
                                      (getenv("PX_NO_ANIMATION") != nullptr && !keep_display_link);
    if (!disable_display_link &&
        CVDisplayLinkCreateWithActiveCGDisplays(&pxw->display_link) == kCVReturnSuccess) {
        CVDisplayLinkSetOutputCallback(pxw->display_link, &display_link_callback, pxw);
        px_mac_update_display_link(pxw);
        CVDisplayLinkStart(pxw->display_link);
    }

    all_windows().push_back(pxw);
    return pxw;
}

void px_destroy_window(px_window_t* window) {
    if (!window) {
        return;
    }
    if (window->display_link) {
        CVDisplayLinkStop(window->display_link);
        CVDisplayLinkRelease(window->display_link);
        window->display_link = nullptr;
    }
    window->window.delegate = nil;
    window->delegate = nil;
    window->view = nil;
    [window->window close];
    window->window = nil;

    std::vector<px_window_t*>& windows = all_windows();
    windows.erase(std::remove(windows.begin(), windows.end(), window), windows.end());
    delete window;
}

#pragma clang diagnostic pop

void px_show_window(px_window_t* window) {
    if (!window) {
        return;
    }

    // makeKeyAndOrderFront: only orders within this application; it does not reliably take focus
    // from the currently active process. This demo calls show() before NSApp enters -run, so do
    // the activation immediately and repeat it on the first main-queue turn after launch. The
    // second call is harmless for windows shown later while the event loop is already running.
    const auto activate = [] {
        [NSRunningApplication.currentApplication
            activateWithOptions:NSApplicationActivateAllWindows |
                                NSApplicationActivateIgnoringOtherApps];
        [NSApp activateIgnoringOtherApps:YES];
    };
    activate();
    [window->window orderFrontRegardless];
    [window->window makeKeyWindow];

    NSWindow* native_window = window->window;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (native_window.isVisible) {
          [NSRunningApplication.currentApplication
              activateWithOptions:NSApplicationActivateAllWindows |
                                  NSApplicationActivateIgnoringOtherApps];
          [NSApp activateIgnoringOtherApps:YES];
          [native_window orderFrontRegardless];
          [native_window makeKeyWindow];
      }
    });
}

void px_show_window_without_focus(px_window_t* window) {
    if (window) {
        [window->window orderFront:nil];
    }
}

void px_hide_window(px_window_t* window) {
    if (window) {
        [window->window orderOut:nil];
    }
}

void px_close_window(px_window_t* window) {
    if (window) {
        [window->window close];
    }
}

void px_set_window_title(px_window_t* window, const char* title) {
    if (window) {
        window->window.title = title ? @(title) : @"";
    }
}

vec2 px_window_size(px_window_t* window) {
    if (!window || !window->view) {
        return vec2{};
    }
    const NSSize size = window->view.bounds.size;
    return vec2{size.width, size.height};
}

vec2 px_window_position(px_window_t* window) {
    if (!window) {
        return vec2{};
    }
    const NSRect frame = window->window.frame;
    const NSRect content = [window->window contentRectForFrameRect:frame];
    // Sublime reports the content rect's top-left, rather than the outer frame's top-left. This
    // lets controls turn an event-local point into screen space by simple addition.
    const CGFloat screen_height = NSScreen.screens.firstObject.frame.size.height;
    return vec2{content.origin.x, screen_height - content.origin.y - content.size.height};
}

void px_set_window_size(px_window_t* window, double width, double height) {
    if (window) {
        [window->window setContentSize:NSMakeSize(width, height)];
    }
}

void px_set_window_position(px_window_t* window, vec2 position) {
    if (!window) {
        return;
    }
    const CGFloat screen_height = NSScreen.screens.firstObject.frame.size.height;
    const NSRect frame = window->window.frame;
    const NSRect content = [window->window contentRectForFrameRect:frame];
    [window->window
        setFrameOrigin:NSMakePoint(position.x, screen_height - position.y - content.size.height)];
}

void px_set_window_maximized(px_window_t* window, bool maximized) {
    if (window && window->window && window->window.isZoomed != maximized) {
        [window->window zoom:nil];
    }
}

double px_window_dpi_scale_factor(px_window_t* window) {
    if (!window || !window->window) {
        return 1.0;
    }
    const CGFloat scale = window->window.backingScaleFactor;
    return scale > 0.0 ? scale : 1.0;
}

void px_set_full_screen(px_window_t* window, bool full_screen) {
    if (!window) {
        return;
    }
    const bool is_full = (window->window.styleMask & NSWindowStyleMaskFullScreen) != 0;
    if (is_full != full_screen) {
        [window->window toggleFullScreen:nil];
    }
}

void px_mark_rect_dirty(px_window_t* window, rect r) {
    if (!window || r.empty()) {
        return;
    }
    window->dirty.push_back(r);
}

void px_mark_dirty(px_window_t* window) {
    if (!window) {
        return;
    }
    const vec2 size = px_window_size(window);
    px_mark_rect_dirty(window, rect{0.0, 0.0, size.x, size.y});
}

void px_set_cursor(px_window_t* window, px_cursor_t cursor) {
    if (!window) {
        return;
    }
    window->cursor = cursor;
    [ns_cursor(cursor) set];
}

void px_reset_cursor(px_window_t* window) {
    if (window) {
        [window->window invalidateCursorRectsForView:window->view];
    }
}

void px_callback_after_event(std::function<void()> fn) {
    post_event_callbacks().push_back(std::move(fn));
}
