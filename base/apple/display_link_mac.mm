#include "base/apple/ca_display_link_mac.h"
#include "base/apple/cv_display_link_mac.h"
#include "base/apple/display_link_mac.h"

namespace base::apple {

std::unique_ptr<DisplayLinkMac> DisplayLinkMac::create_for_display(CGDirectDisplayID display_id) {
    // CADisplayLink is available only for MacOS 14.0+.
    if (@available(macos 14.0, *)) {
        return CADisplayLinkMac::create_for_display(display_id);
    } else {
        return CVDisplayLinkMac::create_for_display(display_id);
    }
}

}  // namespace base::apple
