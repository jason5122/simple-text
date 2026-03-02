#include "base/message_loop/message_pump.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
#include "base/message_loop/message_pump_mac.h"
#endif

namespace base {

MessagePump::MessagePump() = default;

MessagePump::~MessagePump() = default;

std::unique_ptr<MessagePump> MessagePump::create() {
#if BUILDFLAG(IS_MAC)
    return message_pump_mac::create();
#endif
    NOTREACHED();
}

}  // namespace base
