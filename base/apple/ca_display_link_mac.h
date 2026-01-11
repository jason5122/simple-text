#pragma once

#include "base/apple/display_link_mac.h"
#include "base/memory/weak_ptr.h"

namespace base::apple {

class CADisplayLinkMac : public DisplayLinkMac {
public:
    static std::unique_ptr<CADisplayLinkMac> create_for_display(CGDirectDisplayID display_id);
    ~CADisplayLinkMac() override;

    void set_callback(TickCallback callback) override;
    void start() override;
    void stop() override;

private:
    explicit CADisplayLinkMac(CGDirectDisplayID display_id);

    TickCallback tick_callback_;

    struct Impl;
    std::unique_ptr<Impl> impl_;

    base::WeakPtrFactory<CADisplayLinkMac> weak_factory_{this};
};

}  // namespace base::apple
