#pragma once

#include <CoreGraphics/CGDirectDisplay.h>
#include <functional>
#include <memory>

namespace base::apple {

class DisplayLinkMac {
public:
    static std::unique_ptr<DisplayLinkMac> create_for_display(CGDirectDisplayID display_id);
    virtual ~DisplayLinkMac() = default;

    using TickCallback = std::function<void(uint64_t)>;
    virtual void set_callback(TickCallback callback) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

}  // namespace base::apple
