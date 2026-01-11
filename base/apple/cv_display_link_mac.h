#pragma once

#include "base/apple/display_link_mac.h"
#include "base/apple/scoped_typeref.h"
#include <CoreGraphics/CGDirectDisplay.h>
#include <QuartzCore/CVDisplayLink.h>
#include <memory>

namespace base::apple {

class CVDisplayLinkMac : public DisplayLinkMac {
public:
    static std::unique_ptr<CVDisplayLinkMac> create_for_display(CGDirectDisplayID display_id);
    ~CVDisplayLinkMac() override;

    void set_callback(TickCallback callback) override;
    void start() override;
    void stop() override;

private:
    explicit CVDisplayLinkMac(base::apple::ScopedTypeRef<CVDisplayLinkRef> display_link);

    // The static callback function called by the CVDisplayLink, on the CVDisplayLink thread.
    static CVReturn display_link_callback(CVDisplayLinkRef display_link_ref,
                                          const CVTimeStamp* now,
                                          const CVTimeStamp* output_time,
                                          CVOptionFlags flags_in,
                                          CVOptionFlags* flags_out,
                                          void* context);

    base::apple::ScopedTypeRef<CVDisplayLinkRef> display_link_;
    bool display_link_is_running_ = false;

    TickCallback tick_callback_;
};

}  // namespace base::apple
