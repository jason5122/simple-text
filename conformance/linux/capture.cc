#include "conformance/capture.h"
#include "conformance/linux/capture_frame.h"
#include "px/linux/px_linux_private.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr double kPollSeconds = 0.002;
constexpr double kGraceSeconds = 0.2;
constexpr double kTimeoutSeconds = 3.0;

using LinuxFrame = linux_capture::Frame;

bool frames_equal(const LinuxFrame* lhs, const LinuxFrame* rhs) {
    return lhs && rhs && *lhs == *rhs;
}

px_window_t* first_capturable_window() {
    // The conformance capture implementation deliberately reads px's private backing store, so
    // its window lookup is in-process only. Walk newest-to-oldest, which is the useful analogue of
    // the front-to-back window-server searches used by the macOS and Windows implementations.
    const std::vector<px_window_t*>& windows = px_linux_all_windows();
    for (auto it = windows.rbegin(); it != windows.rend(); ++it) {
        px_window_t* window = *it;
        if (window && !window->closing && window->window &&
            gtk_widget_get_visible(window->window)) {
            return window;
        }
    }
    return nullptr;
}

}  // namespace

namespace capture {

Frame capture_frame(WindowId window_id, Crop crop) {
    auto* window = reinterpret_cast<px_window_t*>(window_id);
    if (!window) {
        return nullptr;
    }

    const bool use_gl = window->fbo != 0 && window->fbo_width > 0 && window->fbo_height > 0;
    const bool use_software = window->software_width > 0 && window->software_height > 0 &&
                              !window->software_pixels.empty();
    if (!use_gl && !use_software) return nullptr;
    const int buffer_width = use_gl ? window->fbo_width : window->software_width;
    const int buffer_height = use_gl ? window->fbo_height : window->software_height;

    const int left = std::clamp(crop.x, 0, buffer_width);
    const int top = std::clamp(crop.y, 0, buffer_height);
    const int requested_width = crop.w > 0 ? crop.w : buffer_width - left;
    const int requested_height = crop.h > 0 ? crop.h : buffer_height - top;
    const int width = std::clamp(requested_width, 0, buffer_width - left);
    const int height = std::clamp(requested_height, 0, buffer_height - top);
    if (width == 0 || height == 0) {
        return nullptr;
    }

    auto* frame = new LinuxFrame{
        .width = width,
        .height = height,
        .rgba = std::vector<std::uint8_t>(static_cast<size_t>(width) *
                                          static_cast<size_t>(height) * 4),
    };
    if (use_software) {
        const auto* source = reinterpret_cast<const std::uint8_t*>(window->software_pixels.data());
        const size_t source_row_bytes = static_cast<size_t>(buffer_width) * 4;
        for (int y = 0; y < height; ++y) {
            const std::uint8_t* source_row = source +
                                             static_cast<size_t>(top + y) * source_row_bytes +
                                             static_cast<size_t>(left) * 4;
            std::uint8_t* destination_row =
                frame->rgba.data() + static_cast<size_t>(y) * static_cast<size_t>(width) * 4;
            for (int x = 0; x < width; ++x) {
                destination_row[x * 4] = source_row[x * 4 + 2];
                destination_row[x * 4 + 1] = source_row[x * 4 + 1];
                destination_row[x * 4 + 2] = source_row[x * 4];
                destination_row[x * 4 + 3] = source_row[x * 4 + 3];
            }
        }
        return frame;
    }

    if (!px_linux_gl_make_current(window)) {
        delete frame;
        return nullptr;
    }
    std::vector<std::uint8_t> bottom_up(frame->rgba.size());

    GLint old_framebuffer = 0;
    GLint old_read_buffer = 0;
    GLint old_pack_alignment = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
    glGetIntegerv(GL_READ_BUFFER, &old_read_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &old_pack_alignment);
    glBindFramebuffer(GL_FRAMEBUFFER, window->fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glFinish();
    glReadPixels(left, buffer_height - top - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                 bottom_up.data());
    const GLenum error = glGetError();
    glPixelStorei(GL_PACK_ALIGNMENT, old_pack_alignment);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(old_framebuffer));
    glReadBuffer(static_cast<GLenum>(old_read_buffer));
    if (error != GL_NO_ERROR) {
        std::fprintf(stderr, "capture: glReadPixels failed: GL error 0x%x\n", error);
        delete frame;
        return nullptr;
    }

    // OpenGL rows have a bottom-left origin; capture::Crop and PNG both use top-left.
    const size_t row_bytes = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        std::copy_n(bottom_up.data() + static_cast<size_t>(height - y - 1) * row_bytes, row_bytes,
                    frame->rgba.data() + static_cast<size_t>(y) * row_bytes);
    }
    return frame;
}

void release_frame(Frame frame) { delete static_cast<LinuxFrame*>(frame); }

bool frame_to_png(Frame frame, const char* out_path) {
    auto* linux_frame = static_cast<LinuxFrame*>(frame);
    return linux_frame && linux_capture::write_png(*linux_frame, out_path);
}

void pump(double seconds) {
    for (px_window_t* window : px_linux_all_windows()) {
        px_linux_flush_dirty_rects(window);
    }

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        bool dispatched = false;
        while (g_main_context_pending(nullptr)) {
            g_main_context_iteration(nullptr, FALSE);
            dispatched = true;
        }
        if (!dispatched) {
            const auto remaining = deadline - std::chrono::steady_clock::now();
            const auto one_millisecond =
                std::chrono::duration_cast<decltype(remaining)>(std::chrono::milliseconds(1));
            std::this_thread::sleep_for(std::min(remaining, one_millisecond));
        }
    }
}

Frame wait_settled(WindowId window_id, Crop crop, Frame baseline) {
    const auto started_at = std::chrono::steady_clock::now();
    Frame last = nullptr;
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count() <
           kTimeoutSeconds) {
        pump(kPollSeconds);
        Frame current = capture_frame(window_id, crop);
        if (!current) {
            continue;
        }
        const bool changed = !baseline || !frames_equal(static_cast<LinuxFrame*>(current),
                                                        static_cast<LinuxFrame*>(baseline));
        const bool stable = last && frames_equal(static_cast<LinuxFrame*>(current),
                                                 static_cast<LinuxFrame*>(last));
        const bool graced =
            stable &&
            std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at).count() >=
                kGraceSeconds;
        if ((changed && stable) || graced) {
            release_frame(last);
            return current;
        }
        release_frame(last);
        last = current;
    }
    return last;
}

WindowId find_window(const char*) { return reinterpret_cast<WindowId>(first_capturable_window()); }

WindowId find_window_for_pid(int pid) {
    if (pid != getpid()) {
        return 0;
    }
    return reinterpret_cast<WindowId>(first_capturable_window());
}

}  // namespace capture
