// The CAOpenGLLayer backing.
//
// ST's is `OpenGLLayer : CAOpenGLLayer`, installed from -[PXView makeBackingLayer]. Everything
// below that looks unusual is copied from what the binary does:
//
//   * copyCGLPixelFormatForDisplayMask: and copyCGLContextForPixelFormat: return process-wide
//     globals, and the matching release methods are literally `ret`. Every window shares one GL
//     context, so glyph atlases and buffers are shared for free.
//
//   * The pixel format request, read out of __TEXT,__const at 0x100509e44, is
//     {kCGLPFAColorSize 24, kCGLPFAAlphaSize 8, kCGLPFAOpenGLProfile 0x4100 (3.2 Core),
//      kCGLPFANoRecovery, kCGLPFAAllowOfflineRenderers, kCGLPFABackingStore, 0}.
//     The persistent FBO below, rather than CA's rotating drawable, is what makes repainting only
//     dirty rectangles deterministic.
//
//   * setAsynchronous:NO. Drawing is driven by setNeedsDisplayInRect: and the display link, not by
//     Core Animation polling canDrawInCGLContext:.
//
//   * Rendering goes into a layer-owned persistent FBO. Its RGBA8 color and 8-bit stencil
//     renderbuffers grow with 30 pixels of slack, matching the binary. Dirty regions update that
//     stable image, then the complete visible area is blitted into CA's current drawable.

#include "experiments/platform/mac/px_mac_private.h"

#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>
#import <mach/mach_time.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <vector>

// CAOpenGLLayer and CGL are deprecated as of macOS 10.14; ST 4200 still ships on them, and mirroring
// that is the point of this experiment.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace {

CGLPixelFormatObj g_pixel_format = nullptr;
CGLContextObj g_context = nullptr;

// Created once, on first use, and never released. ST does the same: the globals live at
// 0x100623578 / 0x100623580 and the layer's release callbacks are no-ops.
void ensure_shared_gl() {
  if (g_context) {
    return;
  }

  const CGLPixelFormatAttribute attributes[] = {
      kCGLPFAColorSize,     static_cast<CGLPixelFormatAttribute>(24),
      kCGLPFAAlphaSize,     static_cast<CGLPixelFormatAttribute>(8),
      kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
      kCGLPFANoRecovery,
      kCGLPFAAllowOfflineRenderers,
      kCGLPFABackingStore,
      static_cast<CGLPixelFormatAttribute>(0),
  };

  GLint npix = 0;
  if (CGLChoosePixelFormat(attributes, &g_pixel_format, &npix) != kCGLNoError || !g_pixel_format) {
    NSLog(@"px: CGLChoosePixelFormat failed");
    return;
  }
  if (CGLCreateContext(g_pixel_format, nullptr, &g_context) != kCGLNoError) {
    NSLog(@"px: CGLCreateContext failed");
    g_context = nullptr;
    return;
  }

  // ST writes zero to kCGLCPSwapInterval immediately after creating the process-wide context
  // (CGLSetParameter parameter 0xde). CAOpenGLLayer owns the actual presentation schedule; leaving
  // drawable swaps themselves synchronized can build a short queue between our eager event-driven
  // redraws and WindowServer.
  const GLint swap_interval = 0;
  CGLSetParameter(g_context, kCGLCPSwapInterval, &swap_interval);
}

class GLRenderContext final : public px_render_context {
 public:
  GLRenderContext(vec2 device_size, double scale) : device_size_(device_size), scale_(scale) {}

  vec2 device_size() const override { return device_size_; }
  double dpi_scale_factor() const override { return scale_; }

  void scissor_box(rect r, int out_xywh[4]) const override {
    // Points, top-left origin -> device pixels, bottom-left origin.
    const double x0 = r.x * scale_;
    const double x1 = r.right() * scale_;
    const double y0 = device_size_.y - r.bottom() * scale_;
    const double y1 = device_size_.y - r.y * scale_;

    const double cx0 = std::clamp(std::floor(x0), 0.0, device_size_.x);
    const double cy0 = std::clamp(std::floor(y0), 0.0, device_size_.y);
    const double cx1 = std::clamp(std::ceil(x1), 0.0, device_size_.x);
    const double cy1 = std::clamp(std::ceil(y1), 0.0, device_size_.y);

    out_xywh[0] = static_cast<int>(cx0);
    out_xywh[1] = static_cast<int>(cy0);
    out_xywh[2] = static_cast<int>(std::max(0.0, cx1 - cx0));
    out_xywh[3] = static_cast<int>(std::max(0.0, cy1 - cy0));
  }

 private:
  vec2 device_size_;
  double scale_;
};

double host_time_seconds(uint64_t ticks) {
  static const mach_timebase_info_data_t timebase = [] {
    mach_timebase_info_data_t value{};
    mach_timebase_info(&value);
    return value;
  }();
  return static_cast<double>(ticks) * static_cast<double>(timebase.numer) /
         static_cast<double>(timebase.denom) / 1'000'000'000.0;
}

double trace_quantile(const double* values, int count, double q) {
  std::vector<double> sorted(values, values + count);
  std::sort(sorted.begin(), sorted.end());
  const double index = q * static_cast<double>(count - 1);
  const int lo = static_cast<int>(std::floor(index));
  const int hi = static_cast<int>(std::ceil(index));
  return sorted[lo] * (1.0 - (index - lo)) + sorted[hi] * (index - lo);
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────────────────────────

@interface PXOpenGLLayer : CAOpenGLLayer {
  px_window_t* _pxw;
  std::vector<rect> _dirty;
  std::mutex _dirtyMutex;
  GLuint _framebuffer;
  GLuint _colorRenderbuffer;
  GLuint _stencilRenderbuffer;
  GLsizei _backingWidth;
  GLsizei _backingHeight;
  bool _traceEnabled;
  bool _finishAfterPresent;
  int _maxFramesInFlight;
  int _presentationFenceIndex;
  GLsync _presentationFences[3];
  bool _tracePrintedTimestamp;
  uint64_t _traceLastMotionSerial;
  int _traceCount;
  int _traceTargetCount;
  int _traceBatchSize;
  double _traceEventToDraw[512];
  double _traceDrawToTarget[512];
  double _traceEventToTarget[512];
  double _traceDrawToReturn[512];
  double _traceEventToReturn[512];
}
- (instancetype)initWithPXW:(px_window_t*)pxw;
- (void)addDirtyRect:(rect)r;
@end

@implementation PXOpenGLLayer

- (instancetype)initWithPXW:(px_window_t*)pxw {
  self = [super init];
  if (self) {
    _pxw = pxw;
    _framebuffer = 0;
    _colorRenderbuffer = 0;
    _stencilRenderbuffer = 0;
    _backingWidth = 0;
    _backingHeight = 0;
    _traceEnabled = getenv("PX_LAG_TRACE") != nullptr;
    _finishAfterPresent = getenv("PX_GL_FINISH_AFTER_PRESENT") != nullptr;
    // Keep one submitted frame outstanding. This gives the CPU and GPU useful overlap without
    // allowing a trivial renderer to run far ahead of WindowServer. ST gets equivalent
    // back-pressure from repeatedly updating its small ring of dynamic GL buffers.
    _maxFramesInFlight = 1;
    if (const char* requested = getenv("PX_GL_MAX_FRAMES_IN_FLIGHT")) {
      _maxFramesInFlight = std::clamp(std::atoi(requested), 0, 3);
    }
    _presentationFenceIndex = 0;
    std::fill(std::begin(_presentationFences), std::end(_presentationFences), nullptr);
    _tracePrintedTimestamp = false;
    _traceLastMotionSerial = 0;
    _traceCount = 0;
    _traceTargetCount = 0;
    _traceBatchSize = 512;
    if (const char* requested = getenv("PX_LAG_TRACE_SAMPLES")) {
      _traceBatchSize = std::clamp(std::atoi(requested), 1, 512);
    }
    // -[OpenGLLayer initWithPXW:] seeds the list with a full-size rect so the first frame is a
    // complete repaint.
    _dirty.push_back(rect{0.0, 0.0, px_window_size(pxw).x, px_window_size(pxw).y});
  }
  return self;
}

- (void)addDirtyRect:(rect)r {
  const std::lock_guard lock(_dirtyMutex);
  _dirty.push_back(r);
}

- (void)dealloc {
  const bool hasPresentationFence =
      std::any_of(std::begin(_presentationFences), std::end(_presentationFences),
                  [](GLsync fence) { return fence != nullptr; });
  if (g_context &&
      (_framebuffer || _colorRenderbuffer || _stencilRenderbuffer || hasPresentationFence)) {
    CGLContextObj previous = CGLGetCurrentContext();
    CGLLockContext(g_context);
    CGLSetCurrentContext(g_context);
    for (GLsync& fence : _presentationFences) {
      if (fence) {
        glDeleteSync(fence);
        fence = nullptr;
      }
    }
    if (_stencilRenderbuffer) glDeleteRenderbuffers(1, &_stencilRenderbuffer);
    if (_colorRenderbuffer) glDeleteRenderbuffers(1, &_colorRenderbuffer);
    if (_framebuffer) glDeleteFramebuffers(1, &_framebuffer);
    CGLSetCurrentContext(previous);
    CGLUnlockContext(g_context);
  }
}

- (CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask {
  (void)mask;
  ensure_shared_gl();
  return g_pixel_format;
}

- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)pf {
  // Intentionally empty: the pixel format is a shared global. `ret`, same as ST.
  (void)pf;
}

- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pf {
  (void)pf;
  ensure_shared_gl();
  return g_context;
}

- (void)releaseCGLContext:(CGLContextObj)ctx {
  // Intentionally empty: the context is a shared global.
  (void)ctx;
}

- (void)drawInCGLContext:(CGLContextObj)ctx
             pixelFormat:(CGLPixelFormatObj)pf
            forLayerTime:(CFTimeInterval)t
             displayTime:(const CVTimeStamp*)ts {
  if (!_pxw || !_pxw->handler) {
    return;
  }

  const double scale = px_window_dpi_scale_factor(_pxw);
  self.contentsScale = scale;

  bool addedTraceSample = false;
  int traceSampleIndex = 0;
  double traceEventTime = 0.0;
  double traceDrawTime = 0.0;
  if (_traceEnabled) {
    const uint64_t motionSerial = _pxw->motion_serial.load(std::memory_order_acquire);
    if (motionSerial != 0 && motionSerial != _traceLastMotionSerial) {
      _traceLastMotionSerial = motionSerial;
      traceEventTime = _pxw->last_motion_event_time.load(std::memory_order_relaxed);
      traceDrawTime = host_time_seconds(mach_absolute_time());
      traceSampleIndex = _traceCount;
      _traceEventToDraw[traceSampleIndex] = (traceDrawTime - traceEventTime) * 1000.0;
      if (ts && (ts->flags & kCVTimeStampHostTimeValid)) {
        const double targetTime = host_time_seconds(ts->hostTime);
        _traceDrawToTarget[_traceTargetCount] = (targetTime - traceDrawTime) * 1000.0;
        _traceEventToTarget[_traceTargetCount] = (targetTime - traceEventTime) * 1000.0;
        ++_traceTargetCount;
      }
      ++_traceCount;
      addedTraceSample = true;
    }
  }

  const vec2 size = px_window_size(_pxw);
  const vec2 device{size.x * scale, size.y * scale};
  const GLsizei width = static_cast<GLsizei>(device.x);
  const GLsizei height = static_cast<GLsizei>(device.y);
  if (width < 1 || height < 1) {
    return;
  }

  // ST's renderer alternates between two VBOs for each dynamic primitive stream and updates them
  // with glBufferSubData. Reusing a still-busy buffer supplies implicit driver back-pressure. The
  // opt-in fence ring models that resource-hazard limit for this glClear-only latency benchmark,
  // which otherwise has no reusable GPU resource capable of bounding frames in flight.
  if (_maxFramesInFlight != 0) {
    GLsync& fence = _presentationFences[_presentationFenceIndex];
    if (fence) {
      for (;;) {
        const GLenum result =
            glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1'000'000'000ULL);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED ||
            result == GL_WAIT_FAILED) {
          break;
        }
      }
      glDeleteSync(fence);
      fence = nullptr;
    }
  }

  std::vector<rect> dirty;
  {
    const std::lock_guard lock(_dirtyMutex);
    dirty.swap(_dirty);
  }
  if (dirty.empty()) {
    dirty.push_back(rect{0.0, 0.0, size.x, size.y});
  }

  _pxw->handler->pre_paint();
  px_mac_dispatch_post_event_callbacks();

  GLint destinationDrawFramebuffer = 0;
  GLint previousReadFramebuffer = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &destinationDrawFramebuffer);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);

  if (!_framebuffer) {
    glGenFramebuffers(1, &_framebuffer);
    glGenRenderbuffers(1, &_colorRenderbuffer);
    glGenRenderbuffers(1, &_stencilRenderbuffer);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  if (_backingWidth < width || _backingHeight < height) {
    _backingWidth = std::max(_backingWidth, width + 30);
    _backingHeight = std::max(_backingHeight, height + 30);

    glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, _backingWidth, _backingHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                              _colorRenderbuffer);

    glBindRenderbuffer(GL_RENDERBUFFER, _stencilRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, _backingWidth, _backingHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              _stencilRenderbuffer);

    // Storage was replaced, so every visible pixel must be reconstructed this frame.
    dirty.clear();
    dirty.push_back(rect{0.0, 0.0, size.x, size.y});
  }

  const bool framebufferComplete =
      glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  if (!framebufferComplete) {
    NSLog(@"px: persistent framebuffer is incomplete; drawing directly to CA drawable");
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(destinationDrawFramebuffer));
  }

  glViewport(0, 0, width, height);

  GLRenderContext rc(device, scale);
  _pxw->handler->paint(&rc, rect{0.0, 0.0, size.x, size.y}, dirty.data(),
                       static_cast<int>(dirty.size()));

  if (framebufferComplete) {
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(destinationDrawFramebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebuffer);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
  glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(destinationDrawFramebuffer));

  _pxw->did_first_paint = true;
  _pxw->last_flush = px_now();

  // Flushes and swaps.
  [super drawInCGLContext:ctx pixelFormat:pf forLayerTime:t displayTime:ts];

  // Diagnostic only. ST does not import glFinish, but serializing the submitted GL work lets the
  // latency probe distinguish a GPU command/surface backlog from a later Core Animation or
  // WindowServer delay. Keep this opt-in: forcing completion normally sacrifices throughput.
  if (_finishAfterPresent) {
    glFinish();
  }
  if (_maxFramesInFlight != 0) {
    _presentationFences[_presentationFenceIndex] =
        glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    _presentationFenceIndex = (_presentationFenceIndex + 1) % _maxFramesInFlight;
  }

  if (addedTraceSample) {
    const double returnTime = host_time_seconds(mach_absolute_time());
    _traceDrawToReturn[traceSampleIndex] = (returnTime - traceDrawTime) * 1000.0;
    _traceEventToReturn[traceSampleIndex] = (returnTime - traceEventTime) * 1000.0;
  }

  if (_traceEnabled && !_tracePrintedTimestamp) {
    _tracePrintedTimestamp = true;
    std::fprintf(stderr,
                 "LAG timestamp flags=%#llx host=%llu video=%lld scale=%d rate=%.3f layer=%.6f\n",
                 ts ? static_cast<unsigned long long>(ts->flags) : 0,
                 ts ? static_cast<unsigned long long>(ts->hostTime) : 0,
                 ts ? static_cast<long long>(ts->videoTime) : 0,
                 ts ? static_cast<int>(ts->videoTimeScale) : 0, ts ? ts->rateScalar : 0.0, t);
  }

  // One terminal write per trace batch, after submission, keeps tracing out of the latency-critical
  // path. PX_LAG_TRACE is intentionally opt-in; PX_LAG_TRACE_SAMPLES shortens controlled probes.
  if (addedTraceSample && _traceCount == _traceBatchSize) {
    std::fprintf(stderr, "LAG n=%d event->draw p50=%.3f p90=%.3f ms", _traceBatchSize,
                 trace_quantile(_traceEventToDraw, _traceCount, 0.5),
                 trace_quantile(_traceEventToDraw, _traceCount, 0.9));
    std::fprintf(stderr,
                 "; draw->return p50=%.3f p90=%.3f; event->return p50=%.3f p90=%.3f ms",
                 trace_quantile(_traceDrawToReturn, _traceCount, 0.5),
                 trace_quantile(_traceDrawToReturn, _traceCount, 0.9),
                 trace_quantile(_traceEventToReturn, _traceCount, 0.5),
                 trace_quantile(_traceEventToReturn, _traceCount, 0.9));
    if (_traceTargetCount != 0) {
      std::fprintf(stderr,
                   "; target-n=%d draw->target p50=%.3f p90=%.3f; "
                   "event->target p50=%.3f p90=%.3f ms",
                   _traceTargetCount, trace_quantile(_traceDrawToTarget, _traceTargetCount, 0.5),
                   trace_quantile(_traceDrawToTarget, _traceTargetCount, 0.9),
                   trace_quantile(_traceEventToTarget, _traceTargetCount, 0.5),
                   trace_quantile(_traceEventToTarget, _traceTargetCount, 0.9));
    }
    std::fputc('\n', stderr);
    _traceCount = 0;
    _traceTargetCount = 0;
  }
}

@end

// ─────────────────────────────────────────────────────────────────────────────────────────────────

CALayer* px_mac_make_gl_layer(px_window_t* window) {
  PXOpenGLLayer* layer = [[PXOpenGLLayer alloc] initWithPXW:window];

  // Not asynchronous: redraws come from setNeedsDisplayInRect: and the display link.
  layer.asynchronous = NO;
  layer.needsDisplayOnBoundsChange = YES;
  layer.opaque = window->background.a >= 1.0f;
  layer.contentsScale = px_window_dpi_scale_factor(window);

  CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
  layer.colorspace = cs;
  CGColorSpaceRelease(cs);

  return layer;
}

void px_mac_gl_layer_add_dirty(CALayer* layer, rect r) {
  if ([layer isKindOfClass:[PXOpenGLLayer class]]) {
    [static_cast<PXOpenGLLayer*>(layer) addDirtyRect:r];
  }
}

#pragma clang diagnostic pop
