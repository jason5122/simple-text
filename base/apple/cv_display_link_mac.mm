#include "base/apple/cv_display_link_mac.h"
#include <spdlog/spdlog.h>

namespace base::apple {

template <>
struct ScopedTypeRefTraits<CVDisplayLinkRef> {
    static CVDisplayLinkRef InvalidValue() { return nullptr; }
    static CVDisplayLinkRef Retain(CVDisplayLinkRef object) { return CVDisplayLinkRetain(object); }
    static void Release(CVDisplayLinkRef object) { CVDisplayLinkRelease(object); }
};

std::unique_ptr<CVDisplayLinkMac> CVDisplayLinkMac::create_for_display(
    CGDirectDisplayID display_id) {
    CVReturn ret = kCVReturnSuccess;
    ScopedTypeRef<CVDisplayLinkRef> display_link;
    ret = CVDisplayLinkCreateWithCGDisplay(display_id, display_link.InitializeInto());

    // In normal conditions, the current display is never zero.
    if ((ret == kCVReturnSuccess) && (CVDisplayLinkGetCurrentCGDisplay(display_link.get()) == 0)) {
        spdlog::error("CVDisplayLinkCreateWithCGDisplay failed (no current display)");
        return nullptr;
    }

    std::unique_ptr<CVDisplayLinkMac> result(new CVDisplayLinkMac(display_link));

    ret = CVDisplayLinkSetOutputCallback(display_link.get(),
                                         &CVDisplayLinkMac::display_link_callback, result.get());
    if (ret != kCVReturnSuccess) {
        spdlog::error("CVDisplayLinkSetOutputCallback failed. CVReturn: {}", ret);
        return nullptr;
    }

    return result;
}

void CVDisplayLinkMac::set_callback(TickCallback callback) {
    tick_callback_ = std::move(callback);
}

CVDisplayLinkMac::CVDisplayLinkMac(base::apple::ScopedTypeRef<CVDisplayLinkRef> display_link)
    : display_link_(display_link) {}

void CVDisplayLinkMac::start() {
    if (!display_link_is_running_) {
        DCHECK(!CVDisplayLinkIsRunning(display_link_.get()));
        CVReturn ret = CVDisplayLinkStart(display_link_.get());
        if (ret != kCVReturnSuccess) {
            spdlog::error("CVDisplayLinkStart failed. CVReturn: {}", ret);
        }

        display_link_is_running_ = true;
    }
}

void CVDisplayLinkMac::stop() {
    if (!display_link_is_running_) {
        DCHECK(!CVDisplayLinkIsRunning(display_link_.get()));
        return;
    }

    CVReturn ret = CVDisplayLinkStop(display_link_.get());
    if (ret != kCVReturnSuccess) {
        spdlog::error("CVDisplayLinkStop failed. CVReturn: {}", ret);
    }

    display_link_is_running_ = false;
}

CVDisplayLinkMac::~CVDisplayLinkMac() {
    if (display_link_is_running_) {
        CVReturn ret = CVDisplayLinkStop(display_link_.get());
        if (ret != kCVReturnSuccess) {
            spdlog::error("CVDisplayLinkStop failed. CVReturn: {}", ret);
        }
    }
}

CVReturn CVDisplayLinkMac::display_link_callback(CVDisplayLinkRef display_link_ref,
                                                 const CVTimeStamp* now,
                                                 const CVTimeStamp* output_time,
                                                 CVOptionFlags flags_in,
                                                 CVOptionFlags* flags_out,
                                                 void* context) {
    auto* display_link = static_cast<CVDisplayLinkMac*>(context);

    // TODO: Consider replacing approach with base::SequencedTaskRunner.
    dispatch_async(dispatch_get_main_queue(), ^{
      if (display_link->tick_callback_) display_link->tick_callback_();
    });
    return kCVReturnSuccess;
}

}  // namespace base::apple
