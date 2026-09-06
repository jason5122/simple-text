#include "experiments/platform/conformance/linux/capture_frame.h"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace linux_capture {
namespace {

unsigned long window_pid(Display* display, Window window) {
    const Atom pid_atom = XInternAtom(display, "_NET_WM_PID", True);
    if (pid_atom == None) return 0;
    Atom actual_type = None;
    int actual_format = 0;
    unsigned long count = 0;
    unsigned long remaining = 0;
    unsigned char* property = nullptr;
    const int status = XGetWindowProperty(display, window, pid_atom, 0, 1, False, XA_CARDINAL,
                                          &actual_type, &actual_format, &count, &remaining,
                                          &property);
    unsigned long pid = 0;
    if (status == Success && actual_type == XA_CARDINAL && actual_format == 32 && count == 1) {
        pid = *reinterpret_cast<unsigned long*>(property);
    }
    if (property) XFree(property);
    return pid;
}

std::string process_name(unsigned long pid) {
    std::ifstream file("/proc/" + std::to_string(pid) + "/comm");
    std::string name;
    std::getline(file, name);
    return name;
}

std::string window_name(Display* display, Window window) {
    char* name = nullptr;
    if (!XFetchName(display, window, &name) || !name) return {};
    std::string result(name);
    XFree(name);
    return result;
}

bool class_matches(Display* display, Window window, const char* owner) {
    if (!owner) return false;
    XClassHint hint{};
    if (!XGetClassHint(display, window, &hint)) return false;
    const bool matched = (hint.res_name && std::strcmp(hint.res_name, owner) == 0) ||
                         (hint.res_class && std::strcmp(hint.res_class, owner) == 0);
    if (hint.res_name) XFree(hint.res_name);
    if (hint.res_class) XFree(hint.res_class);
    return matched;
}

bool matches(Display* display, Window window, int pid, const char* owner) {
    const unsigned long candidate_pid = window_pid(display, window);
    if (pid > 0) return candidate_pid == static_cast<unsigned long>(pid);
    if (!owner) return false;
    if (candidate_pid != 0 && process_name(candidate_pid) == owner) return true;
    // _NET_WM_PID is optional and is commonly absent on small X11/XWayland clients.
    return class_matches(display, window, owner) || window_name(display, window) == owner;
}

Window find_window(Display* display, int pid, const char* owner) {
    const Window root = DefaultRootWindow(display);
    const Atom clients_atom = XInternAtom(display, "_NET_CLIENT_LIST_STACKING", True);
    if (clients_atom != None) {
        Atom actual_type = None;
        int actual_format = 0;
        unsigned long count = 0;
        unsigned long remaining = 0;
        unsigned char* property = nullptr;
        if (XGetWindowProperty(display, root, clients_atom, 0, 65536, False, XA_WINDOW,
                               &actual_type, &actual_format, &count, &remaining,
                               &property) == Success &&
            actual_type == XA_WINDOW && actual_format == 32) {
            auto* windows = reinterpret_cast<Window*>(property);
            for (unsigned long i = count; i > 0; --i) {
                if (matches(display, windows[i - 1], pid, owner)) {
                    const Window result = windows[i - 1];
                    XFree(property);
                    return result;
                }
            }
        }
        if (property) XFree(property);
    }
    return None;
}

int mask_shift(unsigned long mask) {
    if (mask == 0) return 0;
    return __builtin_ctzl(mask);
}

std::uint8_t channel(unsigned long pixel, unsigned long mask) {
    if (mask == 0) return 0;
    const unsigned long value = (pixel & mask) >> mask_shift(mask);
    const unsigned long maximum = mask >> mask_shift(mask);
    return static_cast<std::uint8_t>((value * 255 + maximum / 2) / maximum);
}

class X11Source final : public Source {
public:
    ~X11Source() override {
        if (display_) XCloseDisplay(display_);
    }

    bool initialize(int pid, const char* owner, std::string* error) {
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            *error = "XOpenDisplay failed";
            return false;
        }
        int event_base = 0;
        int error_base = 0;
        if (!XCompositeQueryExtension(display_, &event_base, &error_base)) {
            *error = "the X server has no Composite extension";
            return false;
        }
        window_ = find_window(display_, pid, owner);
        if (window_ == None) {
            *error = "could not find the requested X11 window";
            return false;
        }
        return true;
    }

    bool next_frame(Frame* frame, Crop crop, std::chrono::milliseconds timeout) override {
        if (timeout.count() > 0) std::this_thread::sleep_for(timeout);
        XWindowAttributes attributes{};
        if (!XGetWindowAttributes(display_, window_, &attributes) ||
            attributes.map_state != IsViewable) {
            return false;
        }

        const int left = std::clamp(crop.x, 0, attributes.width);
        const int top = std::clamp(crop.y, 0, attributes.height);
        const int width = std::clamp(crop.width > 0 ? crop.width : attributes.width - left, 0,
                                     attributes.width - left);
        const int height = std::clamp(crop.height > 0 ? crop.height : attributes.height - top, 0,
                                      attributes.height - top);
        if (width == 0 || height == 0) return false;

        // A compositing manager already redirects normal top-level windows. Naming that backing
        // pixmap reads the window independently of its position, stacking, and occlusion.
        const Pixmap pixmap = XCompositeNameWindowPixmap(display_, window_);
        if (pixmap == None) return false;
        XImage* image = XGetImage(display_, pixmap, left, top, static_cast<unsigned int>(width),
                                  static_cast<unsigned int>(height), AllPlanes, ZPixmap);
        XFreePixmap(display_, pixmap);
        if (!image) return false;

        frame->width = width;
        frame->height = height;
        frame->rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        // XGetImage() can leave the masks unset for a Pixmap because pixmaps carry a depth but no
        // Visual. The source window's Visual describes the channel layout of its backing pixmap.
        const unsigned long red_mask = attributes.visual->red_mask;
        const unsigned long green_mask = attributes.visual->green_mask;
        const unsigned long blue_mask = attributes.visual->blue_mask;
        const unsigned long color_mask = red_mask | green_mask | blue_mask;
        const unsigned long pixel_mask =
            image->bits_per_pixel >= static_cast<int>(sizeof(unsigned long) * 8)
                ? ~0UL
                : ((1UL << image->bits_per_pixel) - 1);
        const unsigned long alpha_mask = pixel_mask & ~color_mask;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const unsigned long pixel = XGetPixel(image, x, y);
                auto* destination =
                    frame->rgba.data() + (static_cast<size_t>(y) * width + x) * 4;
                destination[0] = channel(pixel, red_mask);
                destination[1] = channel(pixel, green_mask);
                destination[2] = channel(pixel, blue_mask);
                destination[3] = attributes.depth == 32 && alpha_mask ? channel(pixel, alpha_mask)
                                                                       : 255;
            }
        }
        XDestroyImage(image);
        return true;
    }

private:
    Display* display_ = nullptr;
    Window window_ = None;
};

}  // namespace

std::unique_ptr<Source> make_x11_source(int pid, const char* owner, std::string* error) {
    auto source = std::make_unique<X11Source>();
    if (!source->initialize(pid, owner, error)) return nullptr;
    return source;
}

}  // namespace linux_capture
